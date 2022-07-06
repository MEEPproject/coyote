#include "sparta/utils/SpartaAssert.hpp"
#include "MemoryCPUWrapper.hpp"

//-- Activate debugging messages for the Memory Tile
//#define DEBUG_MEMTILE

#ifdef DEBUG_MEMTILE
	#define DEBUG_MSG(str) do {std::cout << getClock()->currentCycle() << ": " << name << " (" << getID() << "): " << __func__ << ": " << str << std::endl;} while(0)
	#define DEBUG_MSG_COLOR(color, str) do {std::cout << color; DEBUG_MSG(str); std::cout << SPARTA_UNMANAGED_COLOR_NORMAL;} while(0)
#else
	#define DEBUG_MSG(str) do {} while(0)
	#define DEBUG_MSG_COLOR(color, str) DEBUG_MSG(str)
#endif

namespace coyote {
	const char MemoryCPUWrapper::name[] = "memory_cpu";

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Constructor
	/////////////////////////////////////////////////////////////////////////////////////////////////

	MemoryCPUWrapper::MemoryCPUWrapper(sparta::TreeNode *node, const MemoryCPUWrapperParameterSet *p):
			sparta::Unit(node),
			line_size(p->line_size),
			latency(p->latency),
			llc_banks(p->num_llc_banks),
			max_vvl(p->max_vvl),
			out_ports_llc(llc_banks),
			in_ports_llc(llc_banks),
			out_ports_llc_mc(llc_banks),
			in_ports_llc_mc(llc_banks),
			sched_mem_req(&controller_cycle_event_mem_req, p->latency),
			sched_outgoing(&controller_cycle_event_outgoing_transaction, p->latency),
			sched_incoming_mc(&controller_cycle_event_incoming_mem_req, 1) {
				in_port_noc.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_noc, std::shared_ptr<NoCMessage>));
				in_port_mc.registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_mc, std::shared_ptr<CacheRequest>));


				for(uint16_t i=0; i<llc_banks; i++)
				{
				    std::string out_name=std::string("out_llc") + sparta::utils::uint32_to_str(i);
				    std::unique_ptr<sparta::DataOutPort<std::shared_ptr<Request>>> out=std::make_unique<sparta::DataOutPort<std::shared_ptr<Request>>> (&unit_port_set_, out_name, false);
				    out_ports_llc[i]=std::move(out);

				    out_name=std::string("out_llc_mc") + sparta::utils::uint32_to_str(i);
				    std::unique_ptr<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> out_ack=std::make_unique<sparta::DataOutPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, out_name, false);
				    out_ports_llc_mc[i]=std::move(out_ack);

				    std::string in_name=std::string("in_llc") + sparta::utils::uint32_to_str(i);
				    std::unique_ptr<sparta::DataInPort<std::shared_ptr<Request>>> in_ack=std::make_unique<sparta::DataInPort<std::shared_ptr<Request>>> (&unit_port_set_, in_name);
				    in_ports_llc[i]=std::move(in_ack);
				    in_ports_llc[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_llc, std::shared_ptr<CacheRequest>));

				    in_name=std::string("in_llc_mc") + sparta::utils::uint32_to_str(i);
				    std::unique_ptr<sparta::DataInPort<std::shared_ptr<CacheRequest>>> in_req=std::make_unique<sparta::DataInPort<std::shared_ptr<CacheRequest>>> (&unit_port_set_, in_name);
				    in_ports_llc_mc[i]=std::move(in_req);
				    in_ports_llc_mc[i]->registerConsumerHandler(CREATE_SPARTA_HANDLER_WITH_DATA(MemoryCPUWrapper, receiveMessage_llc_mc, std::shared_ptr<CacheRequest>));
				}

				if(p->llc_pol=="page_to_bank")
				{
				    llc_policy=coyote::CacheDataMappingPolicy::PAGE_TO_BANK;
				}
				else if(p->llc_pol=="set_interleaving")
				{
				    llc_policy=coyote::CacheDataMappingPolicy::SET_INTERLEAVING;
				}
				else
				{
				    printf("Unsupported cache data mapping policy\n");
				}

				instructionID_counter = 1;
				this->id = 0;
								
				sp_status = (SPStatus *)calloc(num_of_registers, sizeof(SPStatus));
				sparta_assert(sp_status != nullptr, "SP status array could not be allocated!");
								
				//-- read the configuration
				root_node			= node->getRoot()->getAs<sparta::RootTreeNode>();
				auto config			= root_node->getSimulator()->getSimulationConfiguration()->getUnboundParameterTree();
				this->enabled		= config.tryGet("meta.params.enable_smart_mcpu")->getAs<bool>();
				this->enabled_llc	= config.tryGet("meta.params.enable_llc")->getAs<bool>();
				this->sp_reg_size	= (size_t)config.tryGet("top.arch.tile0.params.num_l2_banks")->getAs<uint16_t>() *
									  (size_t)config.tryGet("top.arch.tile0.l2_bank0.params.size_kb")->getAs<uint64_t>() *
									  (size_t)1024 / (
									  	(size_t)config.tryGet("top.arch.params.num_cores")->getAs<uint16_t>() /
									  	(size_t)config.tryGet("top.arch.params.num_tiles")->getAs<uint16_t>()
									  ) /
									  (size_t)num_of_registers;
				uint16_t n_cores	= config.tryGet("top.arch.params.num_cores")->getAs<uint16_t>();
				this->vvl			= (uint32_t *)std::calloc(n_cores, sizeof(uint32_t));
				sparta_assert(this->vvl != nullptr, "Could not allocate the array to hold VVL in the Memory Tile.");
				
				
				DEBUG_MSG("Memory Tile is " << (this->enabled ? "enabled" : "disabled") << ".");					  
				if(enabled) {
					DEBUG_MSG("Each register entry in the SP is " << sp_reg_size << " Bytes.");
				}
				
				this->noc      = nullptr; 
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////
	//-- Message handling from the NoC
	/////////////////////////////////////////////////////////////////////////////////////////////////
	bool MemoryCPUWrapper::ableToReceivePacket(const std::shared_ptr<NoCMessage> &mes) {

		//-- the MCPU handles a packet currently
		if(mes->getType() == NoCMessageType::MCPU_REQUEST && transaction_table.size() > 0) {
			return false;
		}
		return true;
	}

	void MemoryCPUWrapper::receiveMessage_noc(const std::shared_ptr<NoCMessage> &mes) {
		count_requests_noc++;
		
		if(trace_) {
		/*	std::shared_ptr<Request> req = std::dynamic_pointer_cast<Request>(mes->getRequest());
            std::cout << getClock()->currentCycle() << ", " << getID() << "," << mes->getSrcPort() << "," << req->getAddress() << "\n";
			logger_->logMemTileNoCRecv(getClock()->currentCycle(), getID(), mes->getSrcPort(), req->getAddress());*/
		}
		
		/*if(!enabled) {
			std::shared_ptr<CacheRequest> cr = std::dynamic_pointer_cast<CacheRequest>(mes->getRequest());
			DEBUG_MSG("CacheRequest received from NoC: " << *cr);
			
			//cr->set_mem_op_latency(line_size/32); // if memtile not enabled, handle only cache lines (64 Bytes)
			
			if(this->enabled_llc) {
				out_ports_llc[calculateBank(cr)]->send(cr, 0);
				if(trace_) {
					logger_->logMemTileLLCSent(getClock()->currentCycle(), getID(), cr->getAddress());
				}
				count_requests_llc++;
			} else {
				out_port_mc.send(cr, 0);
				if(trace_) {
					logger_->logMemTileMCSent(getClock()->currentCycle(), getID(), cr->getAddress());
				}
				count_requests_mc++;
			}
			
			return;
		}*/
		
		DEBUG_MSG("Received from NoC: " << *mes);	
		mes->getRequest()->handle(this);
	}


	//-- Bypass for a scalar memory operation
	void MemoryCPUWrapper::handle(std::shared_ptr<coyote::CacheRequest> mes) {
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_CYAN, "CacheRequest: " << *mes);
				
		if(mes->getMemTile() == (uint16_t)-1) {		//-- a transaction from the VAS Tile
			//mes->set_mem_op_latency(line_size/32);
            mes->setSize(line_size);
			sendToDestination(mes);					//-- check, if the address is for the local memory or for a remote MemTile
			count_scalar++;

            if(trace_) {
			    logger_->logMemTileScaOpRecv(getClock()->currentCycle(), getID(), mes->getCoreId(), mes->getAddress());
            }
		} else {
			count_received_other_memtile++;
			if(mes->isServiced()) {					//-- this transaction has been completed by a different memory tile.
				DEBUG_MSG("\tServiced by a different Memory Tile");
				handleReplyMessageFromMC(mes);
                if(trace_) {
				    logger_->logMemTileMTOpRecv(getClock()->currentCycle(), getID(), calcDestMemTile(mes->getAddress()), mes->getAddress());
                }
			} else {								//-- the parent transaction has been received by a different memory tile, but it is served here.
				DEBUG_MSG("\tSource is a different Memory Tile: " << mes->getMemTile());
				sched_mem_req.push(mes);
				log_sched_mem_req();
			}
		}
	}



	//-- A memory transaction to be handled by the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<coyote::MCPUSetVVL> mes) {
		
		count_control++;
		
		uint64_t elements_per_sp = max_vvl / ((uint64_t)(mes->getWidth())*8);
		vvl[mes->getCoreId()] = std::min(elements_per_sp, mes->getAVL());
		
		int lmul = (int)mes->getLMUL();
		if(lmul < 0) {
			vvl[mes->getCoreId()] >>= lmul*(-1);
		} else {
			vvl[mes->getCoreId()] <<= lmul;
		}
		
		mes->setVVL(vvl[mes->getCoreId()]);
		mes->setServiced();
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN, "MCPUSetVVL: " << *mes);
			
		std::shared_ptr<NoCMessage> outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MCPU_REQUEST, 8, getID(), mes->getSourceTile());
		sched_outgoing.push(outgoing_noc_message);
		if(trace_) {
			logger_->logMemTileVVL(getClock()->currentCycle(), getID(), mes->getCoreId(), vvl[mes->getCoreId()]);
			log_sched_outgoing();
		}
	}



	//-- A vector instruction for the MCPU
	void MemoryCPUWrapper::handle(std::shared_ptr<coyote::MCPUInstruction> instr) {
	
		count_vector++;
        if(trace_) {
		    logger_->logMemTileVecOpRecv(getClock()->currentCycle(), getID(), instr->getCoreId(), instr->getAddress());
		}
		
		instr->setID(this->instructionID_counter);
		struct Transaction instruction_attributes = {instr, 0, 0, 1, vvl[instr->getCoreId()]};
		transaction_table.insert({
						this->instructionID_counter,
						instruction_attributes
		}); // keep track of the transaction
		
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN, "Memory instruction received: " << *instr);
		
		
		if(instr->get_operation() == MCPUInstruction::Operation::LOAD) {
			
			//-- If we have not done before, an ALLOCATE has to be sent to the VAS Tile, so it reserves space for the data to be written to the SP
			if(sp_status[instr->getDestinationRegId()] == SPStatus::IS_L2) {
				sp_status[instr->getDestinationRegId()] = SPStatus::ALLOC_SENT;
				
				std::shared_ptr<ScratchpadRequest> sp_request = createScratchpadRequest(instr, ScratchpadRequest::ScratchpadCommand::ALLOCATE);
				sp_request->setSize(vvl[instr->getCoreId()] * (uint)instr->get_width());	// reserve the space in the VAS Tile
				std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(sp_request, NoCMessageType::SCRATCHPAD_COMMAND, 15, getID(), instr->getSourceTile());
				sched_outgoing.push(noc_message);
				
				if(trace_) {
			    	logger_->logMemTileSPOpSent(getClock()->currentCycle(), getID(), instr->getSourceTile(), instr->getAddress());
			    	log_sched_outgoing();
                }
				
				DEBUG_MSG("sending SP ALLOC: " << *sp_request);
			}
			
			//-- if it is an indexed load, load the indices
			if(instr->get_suboperation() == MCPUInstruction::SubOperation::ORDERED_INDEX || instr->get_suboperation() == MCPUInstruction::SubOperation::UNORDERED_INDEX) {
				
				instruction_attributes.counter_scratchpadRequests = 1;	// one returning SP Request is expected
				transaction_table.at(this->instructionID_counter) = instruction_attributes;
				
				std::shared_ptr<ScratchpadRequest> sp_request = createScratchpadRequest(instr, ScratchpadRequest::ScratchpadCommand::READ);
				sp_request->setSize(vvl[instr->getCoreId()] * (uint)instr->get_width());	// reserve the space in the VAS Tile
				std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(sp_request, NoCMessageType::SCRATCHPAD_COMMAND, 15, getID(), instr->getSourceTile());
				sched_outgoing.push(noc_message);
				
				if(trace_) {
			    	logger_->logMemTileSPOpSent(getClock()->currentCycle(), getID(), instr->getSourceTile(), instr->getAddress());
			    	log_sched_outgoing();
                }
				
				DEBUG_MSG("sending SP READ: " << *sp_request);	
			} else {
				//-- Send the request to the Vector Address Generators (VAG)
				computeMemReqAddresses(instr);
			}
			
			
			
		} else { // If it is a store, generate a ScratchpadRequest to load the data from the VAS Tile
			
			instruction_attributes.counter_scratchpadRequests = 1;
			if(instr->get_suboperation() == MCPUInstruction::SubOperation::ORDERED_INDEX || instr->get_suboperation() == MCPUInstruction::SubOperation::UNORDERED_INDEX) {
					instruction_attributes.counter_scratchpadRequests = 2;	// one to read indices, the other one to read the data
			}
			transaction_table.at(this->instructionID_counter) = instruction_attributes;
			
			for(uint32_t i=0; i<instruction_attributes.counter_scratchpadRequests; ++i) {
				std::shared_ptr outgoing_message = createScratchpadRequest(instr, ScratchpadRequest::ScratchpadCommand::READ);
				outgoing_message->setSize(vvl[instr->getCoreId()] * (uint)instr->get_width());	// How many bytes to read from the SP?
			
			std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(outgoing_message, NoCMessageType::SCRATCHPAD_COMMAND, 15, getID(), instr->getSourceTile());
			
				sched_outgoing.push(noc_message);
				if(trace_) {
				    logger_->logMemTileSPOpSent(getClock()->currentCycle(), getID(), instr->getSourceTile(), instr->getAddress());
				    log_sched_outgoing();
                }
			
				DEBUG_MSG("sending SP READ: " << *outgoing_message);
			}
		}
		
		this->instructionID_counter++; // increment ID
		if(this->instructionID_counter == 0) {this->instructionID_counter = 1;}	// handle the overflow. 0 is reserved for cache requests that use the bypass
	}


	//-- Handle for Scratchpad requests
	void MemoryCPUWrapper::handle(std::shared_ptr<coyote::ScratchpadRequest> instr) {
			
		count_sp_requests++;
		
		std::unordered_map<std::uint32_t, Transaction>::iterator transaction_id = transaction_table.find(instr->getID());
		std::shared_ptr<coyote::MCPUInstruction> parent_instr = transaction_id->second.mcpu_instruction;
		
		DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_BRIGHT_CYAN, "ScratchpadRequest: " << *instr << ", parent instr: " << *parent_instr);
        if(trace_) {
			logger_->logMemTileSPOpRecv(getClock()->currentCycle(), getID(), instr->getSourceTile(), parent_instr->getAddress());
        }

		switch(instr->getCommand()) {
			case ScratchpadRequest::ScratchpadCommand::ALLOCATE:
				sched_outgoing.notify(parent_instr->getDestinationRegId());
				sp_status[parent_instr->getDestinationRegId()] = SPStatus::READY;
				break;
			case ScratchpadRequest::ScratchpadCommand::FREE:
				break;
			case ScratchpadRequest::ScratchpadCommand::READ: // this READ is a reply to the READ command. It contains the data to be stored in memory
				if(instr->isOperandReady()) {
					//-- send the data to the MC only once. In case of indexed vector stores 2 READ requests are sent to the VAS Tile:
					//   1) read the indices
					//   2) read the data
					//   However, only the data should be stored.
					transaction_id->second.counter_scratchpadRequests--;
					if(transaction_id->second.counter_scratchpadRequests == 0) {
						computeMemReqAddresses(parent_instr);
					}
				}
				break;
			case ScratchpadRequest::ScratchpadCommand::WRITE:
				if(instr->isOperandReady()) {
					computeMemReqAddresses(parent_instr);
				}
				break;
			default:
				sparta_assert(false, "The Memory Tile does not understand this Scratchpad command!");
		}
	}

	
	
	//-- Schedule the memory operations going to the MC
	void MemoryCPUWrapper::controllerCycle_mem_requests() {

		//-- Get the oldest Cache Request from the queue
		std::shared_ptr<CacheRequest> instr_for_mc = sched_mem_req.front();
		
		//-- Send to the LLC or MC
		if(this->enabled_llc) {
			//instr_for_mc->set_mem_op_latency(line_size/32);	// Overwrite! LLC always loads 64 Bytes = 2*32 Bytes
			out_ports_llc[calculateBank(instr_for_mc)]->send(instr_for_mc, 1);
			
			if(trace_) {
				logger_->logMemTileLLCSent(getClock()->currentCycle(), getID(), instr_for_mc->getAddress(), getParentAddress(instr_for_mc));
			}
			DEBUG_MSG("Sending to LLC: " << *instr_for_mc);
			count_requests_llc++;
		} else {
			//-- set_mem_op_latency set in the Vector Address Generator
			out_port_mc.send(instr_for_mc, 1); 	// This latency needs to be >0. Otherwise, a lower priority event would trigger 
												// a higher priority one for the same cycle, 
												// being the scheduling phase for the higher one already finished.
			if(trace_) {
				logger_->logMemTileMCSent(getClock()->currentCycle(), getID(), instr_for_mc->getAddress(), getParentAddress(instr_for_mc));
			}
			DEBUG_MSG("Sending to MC: " << *instr_for_mc);
			count_requests_mc++;
		}
		
		//-- consume the memory request from the scheduler
		sched_mem_req.pop();
		log_sched_mem_req();
	}
	
	
	void MemoryCPUWrapper::controllerCycle_outgoing_transaction() {

		sparta_assert(noc, "The MCPU does not have access to methods of the NoC, since MemoryCPUWrapper::setNoC(coyote::NoC) was not called.");
		
		std::shared_ptr<NoCMessage> response = sched_outgoing.front();	
		if(noc->checkSpaceForPacket(false, response)) {
			out_port_noc.send(response, 0);
			count_replies_noc++;
			
			sched_outgoing.pop();
			DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_GREEN, "Sending to NoC from " << getID() << ": " << *response);
			if(trace_) {
				/*std::shared_ptr<Request> req = std::dynamic_pointer_cast<Request>(response->getRequest());
				logger_->logMemTileNoCSent(getClock()->currentCycle(), getID(), response->getDstPort(), req->getAddress());
				log_sched_outgoing();
                */
			}
			
			
		} else {
			DEBUG_MSG_COLOR(SPARTA_UNMANAGED_COLOR_GREEN, "NoC does not accept message: " << *response);
			count_replies_wait_noc++;

			//-- reschedule for the next cycle
			sched_outgoing.reschedule();
		}
	}
	
	
	
	
	
	
	
	
	
	
	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	//-- ADDRESS GENERATION
	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	void MemoryCPUWrapper::memOp_unit(std::shared_ptr<MCPUInstruction> instr) {
		std::unordered_map<std::uint32_t, Transaction>::iterator transaction_id = transaction_table.find(instr->getID());
		uint32_t local_vvl = transaction_id->second.vvl;
				
		//-- get the number of elements loaded by 1 memory request
		uint number_of_elements_per_request = line_size / (uint32_t)instr->get_width();
		int32_t remaining_elements = local_vvl;
		uint32_t number_of_replies = 0;
		uint64_t address = instr->getAddress();
		
		
		while(remaining_elements > 0) {
			
			//-- Generate a cache line request
			std::shared_ptr<CacheRequest> memory_request = createCacheRequest(address, instr);
			
			DEBUG_MSG("elements/request: " << number_of_elements_per_request << ", remaining elements: " << remaining_elements << ", CR: " << *memory_request);
			//memory_request->set_mem_op_latency(line_size/32);	// load 64 Bytes
            memory_request->setSize(line_size);
									
			//-- schedule this request for the MC
			sendToDestination(memory_request);
			
			remaining_elements -= number_of_elements_per_request;
			address += line_size;
			number_of_replies++;
		}		
		
		transaction_id->second.counter_cacheRequests = number_of_replies;		// How many responses are expected from the MC?
		transaction_id->second.counter_scratchpadRequests = number_of_replies;	// How many responses are sent back to the VAS Tile?
		transaction_id->second.number_of_elements_per_response = 1; 			// Send out every received CacheRequest from the MC (no parasitic bytes)
	}
	
	void MemoryCPUWrapper::memOp_nonUnit(std::shared_ptr<MCPUInstruction> instr) {
		std::unordered_map<std::uint32_t, Transaction>::iterator transaction_id = transaction_table.find(instr->getID());
		uint32_t local_vvl = transaction_id->second.vvl;
		
		std::vector<uint64_t> indices = instr->get_index();
		uint64_t address = instr->getAddress();
		
		for(std::vector<uint64_t>::iterator it = indices.begin(); it != indices.end(); ++it) {
			DEBUG_MSG("index: 0x" << *it << ", base_address: 0x" << address << ", computed_address: 0x" << address + *it);	
			//-- create and schedule request for the MC
			std::shared_ptr<CacheRequest> memory_request = createCacheRequest(address + *it, instr);
			//memory_request->set_mem_op_latency(1);	// load 32 Bytes or less
            memory_request->setSize(32); //This is set to the smallest READ for HBM
			sendToDestination(memory_request);
		}
		
		uint32_t number_of_elements_per_response = line_size / (uint32_t)instr->get_width();
		
		transaction_id->second.counter_cacheRequests = local_vvl;				// How many responses are expected from the MC?
		transaction_id->second.counter_scratchpadRequests = (local_vvl + number_of_elements_per_response - 1)/number_of_elements_per_response;	// How many responses are sent back to the VAS Tile? (ceil(local_vvl/number_of_elements_per_response), but with integers)
		transaction_id->second.number_of_elements_per_response = number_of_elements_per_response; 		// How many elements fit into 1 line_size (64 Bytes)?
	
		DEBUG_MSG("summary: expected CRs: " << local_vvl << ", SPs to return: " << transaction_id->second.counter_scratchpadRequests << ", elements per MC response: " << number_of_elements_per_response);	
	}
	
	void MemoryCPUWrapper::memOp_orderedIndex(std::shared_ptr<MCPUInstruction> instr) {
		memOp_nonUnit(instr);	// there is no difference
	}
	
	void MemoryCPUWrapper::memOp_unorderedIndex(std::shared_ptr<MCPUInstruction> instr) {
		memOp_nonUnit(instr);	// there is no difference
	}





	/////////////////////////////////////////////////////////////////////////////////////////////////
	//-- Message handling from the MC
	/////////////////////////////////////////////////////////////////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_mc(const std::shared_ptr<CacheRequest> &mes)	{

		if(this->enabled_llc) {
			out_ports_llc_mc[calculateBank(mes)]->send(mes, 0);
			
			if(trace_) {
				logger_->logMemTileMC2LLC(getClock()->currentCycle(), getID(), mes->getAddress());
			}
			
		} else {
			sched_incoming_mc.push(mes);
			if(trace_) {
				logger_->logMemTileMCRecv(getClock()->currentCycle(), getID(), mes->getAddress());
			}
		}
	}
	
	
	void MemoryCPUWrapper::controllerCycle_incoming_mem_req() {
		
		std::shared_ptr<CacheRequest> mes = sched_incoming_mc.front();
		sched_incoming_mc.pop();
		
		mes->setMemoryAck(true);
		
		DEBUG_MSG("Returned from MC: " << *mes);
		if(!enabled) {
			//-- whatever we received from the MC, just forward it to the NoC
            uint16_t destination=mes->getHomeTile();
            if(mes->getBypassL2())
            {
                destination=mes->getSourceTile();
                mes->setServiced();
            }

			std::shared_ptr<NoCMessage> outgoing_noc_message;
			outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size, getID(), destination);
			sched_outgoing.push(outgoing_noc_message);
			if(trace_) {
				log_sched_outgoing();
			}
			
			return;
		}
	
		mes->setServiced();
		
		//-- The returning CacheRequest was sent to this MemTile by another MemTile. Just return the ACK to the originating MemTile
		if(mes->getMemTile() != (uint16_t)-1) {
			std::shared_ptr<NoCMessage> outgoing_noc_message;
			outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEM_TILE_REPLY, line_size, getID(), mes->getMemTile());
			sched_outgoing.push(outgoing_noc_message);
			if(trace_) {
				logger_->logMemTileMTOpSent(getClock()->currentCycle(), getID(), mes->getMemTile(), mes->getAddress());
				log_sched_outgoing();
            }
			return;
		}
		
		handleReplyMessageFromMC(mes);
	}





	/////////////////////////////////////////////////////////////////////////////////////////////////
	//-- Message Handling from the LLC
	/////////////////////////////////////////////////////////////////////////////////////////////////
	void MemoryCPUWrapper::receiveMessage_llc(const std::shared_ptr<CacheRequest> &mes) {
		sched_incoming_mc.push(mes);
		if(trace_) {
			logger_->logMemTileLLCRecv(getClock()->currentCycle(), getID(), mes->getAddress(), getParentAddress(mes));
		}
	}
	
	//-- message from the LLC for the MC
	void MemoryCPUWrapper::receiveMessage_llc_mc(const std::shared_ptr<CacheRequest> &mes) {
		out_port_mc.send(mes, 0);
		count_requests_mc++;
		
		if(trace_) {
			logger_->logMemTileLLC2MC(getClock()->currentCycle(), getID(), mes->getAddress(), getParentAddress(mes));
		}
	}






	/////////////////////////////////////////////////////////////////////////////////////////////////
	//-- Helper functions
	/////////////////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<ScratchpadRequest> MemoryCPUWrapper::createScratchpadRequest(const std::shared_ptr<MCPUInstruction> &mes, ScratchpadRequest::ScratchpadCommand command) {
		std::shared_ptr<ScratchpadRequest> sp_request = std::make_shared<ScratchpadRequest>(
						mes->getAddress(),
						command,
						mes->getPC(),
						mes->getTimestamp(),
						mes->getCoreId(),
						getID(),
						mes->getDestinationRegId()
		);
		sp_request->setID(mes->getID());
		return sp_request;
	}
	
    std::shared_ptr<ScratchpadRequest> MemoryCPUWrapper::createScratchpadRequest(const std::shared_ptr<Request> &mes, ScratchpadRequest::ScratchpadCommand command) {
		std::shared_ptr<ScratchpadRequest> sp_request = std::make_shared<ScratchpadRequest>(
						mes->getAddress(),
						command,
						mes->getPC(),
						mes->getTimestamp(),
						mes->getCoreId(),
						getID(),
						mes->getDestinationRegId()
		);
		sp_request->setID(mes->getID());
		return sp_request;
	}
	
	
	std::shared_ptr<CacheRequest> MemoryCPUWrapper::createCacheRequest(uint64_t address, std::shared_ptr<MCPUInstruction> instr) {
		std::shared_ptr<CacheRequest> cr = std::make_shared<CacheRequest>(
					address,
					(instr->get_operation() == MCPUInstruction::Operation::LOAD) ? 
								CacheRequest::AccessType::LOAD : CacheRequest::AccessType::STORE,
					instr->getPC(),
					instr->getTimestamp(), 
					instr->getCoreId());

		cr->setDestinationReg(instr->getDestinationRegId(), instr->getDestinationRegType());
		cr->setID(instr->getID());	// set CacheRequest ID to the mcpu instruction ID it was generated from
		return cr;
	}
	
	
	void MemoryCPUWrapper::computeMemReqAddresses(std::shared_ptr<MCPUInstruction> instr) {
		switch(instr->get_suboperation()) {
			case MCPUInstruction::SubOperation::UNIT:
				memOp_unit(instr);
				break;
			case MCPUInstruction::SubOperation::NON_UNIT:
				memOp_nonUnit(instr);
				break;
			case MCPUInstruction::SubOperation::ORDERED_INDEX:
				memOp_orderedIndex(instr);
				break;
			case MCPUInstruction::SubOperation::UNORDERED_INDEX:
				memOp_unorderedIndex(instr);
				break;
			default:
				sparta_assert(false, "UNKNOWN MCPUInstruction SubOperation. The MCPU does not understand, what that instruction means.");
		}
	}


	void MemoryCPUWrapper::setAddressMappingInfo(uint64_t memory_controller_shift, uint64_t memory_controller_mask) {
		mc_shift = memory_controller_shift;
		mc_mask  = memory_controller_mask;
	}


	uint16_t MemoryCPUWrapper::calcDestMemTile(uint64_t address) {
		return (uint16_t)((address >> mc_shift) & mc_mask);
	}

	
	void MemoryCPUWrapper::handleReplyMessageFromMC(std::shared_ptr<CacheRequest> mes) {
		
		//-- Get the ID of the instruction
		//   If the ID is = 0, the CacheRequest utilizes the Bypass (scalar operation)
		//   Otherwise, the CacheRequest was generated in a Memory Tile and hence a parent instruction exists.
		if(mes->getID() == 0) {
			DEBUG_MSG("\tCacheRequest using the Bypass: " << *mes);
			
			std::shared_ptr<NoCMessage> outgoing_noc_message;
			outgoing_noc_message = std::make_shared<NoCMessage>(mes, NoCMessageType::MEMORY_ACK, line_size, mes->getMemoryController(), mes->getHomeTile());
			
			sched_outgoing.push(outgoing_noc_message);
			
            if(trace_) {
			    logger_->logMemTileScaOpSent(getClock()->currentCycle(), getID(), mes->getHomeTile(), mes->getAddress());
				log_sched_outgoing();
            }
			return;
		}

		
		//-- A CacheRequest that has been generated by an MCPUInstruction
		std::unordered_map<std::uint32_t, Transaction>::iterator transaction_id = transaction_table.find(mes->getID());
		sparta_assert(transaction_id != transaction_table.end(), "Could not find parent instruction for instruction " << *mes);
		
		DEBUG_MSG("\tHandled in the MCPU: parent instr: " << *(transaction_id->second.mcpu_instruction));
		
		transaction_id->second.counter_cacheRequests--;
		uint32_t cacheRequests_to_go_for_SP = transaction_id->second.counter_cacheRequests % transaction_id->second.number_of_elements_per_response;
		
		switch(mes->getType())	{
			case CacheRequest::AccessType::FETCH:
			case CacheRequest::AccessType::LOAD:
				
				if(cacheRequests_to_go_for_SP == 0) {
					transaction_id->second.counter_scratchpadRequests--;
					
					std::shared_ptr outgoing_message = createScratchpadRequest(mes, ScratchpadRequest::ScratchpadCommand::WRITE);
					outgoing_message->setSize(line_size);
					outgoing_message->setOperandReady(transaction_id->second.counter_scratchpadRequests == 0);
					
					std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(outgoing_message, NoCMessageType::SCRATCHPAD_COMMAND, line_size, getID(), transaction_id->second.mcpu_instruction->getSourceTile());
					
					
					//-- Check if the SP is already allocated. If not, store the msg in a delaying queue
					size_t destReg = transaction_id->second.mcpu_instruction->getDestinationRegId();
					if(sp_status[destReg] == SPStatus::READY) {
						sched_outgoing.push(noc_message);
					} else {
						sched_outgoing.add_delay_queue(noc_message, destReg);
					}
					
                    if(trace_) {
						logger_->logMemTileSPOpSent(getClock()->currentCycle(), getID(), transaction_id->second.mcpu_instruction->getSourceTile(), transaction_id->second.mcpu_instruction->getAddress());
						log_sched_outgoing();
                    }
					DEBUG_MSG("\t\tReturning SP: " << *outgoing_message);
				}
				break;
			case CacheRequest::AccessType::WRITEBACK:
			case CacheRequest::AccessType::STORE:
				//-- Do not sent an ACK, not required. Nothing else to do here.
				break;
		}
		
		DEBUG_MSG("\t\tCRs/SPRs to go: " << transaction_id->second.counter_cacheRequests << "/" << transaction_id->second.counter_scratchpadRequests);
		
		if(transaction_id->second.counter_cacheRequests == 0) {
			DEBUG_MSG("\t\t\tcomplete, deleted instr: " << *(transaction_id->second.mcpu_instruction));
			transaction_table.erase(transaction_id);
		}
	}
	
	
	void MemoryCPUWrapper::sendToDestination(std::shared_ptr<CacheRequest> mes) {
		uint16_t destMemTile = calcDestMemTile(mes->getAddress());
		if(destMemTile == getID()) {				//-- the address range is served by the current memory tile
			//-- Schedule memory request for the MC
			sched_mem_req.push(mes);
			log_sched_mem_req();
		} else {									//-- the address range is not served by the current memory tile
			mes->setMemTile(getID());
			std::shared_ptr<NoCMessage> noc_message = std::make_shared<NoCMessage>(
					mes,
					NoCMessageType::MEM_TILE_REQUEST,
					line_size,
					getID(),
					destMemTile
			);
			count_send_other_memtile++;
			sched_outgoing.push(noc_message);
            if(trace_) {
			    logger_->logMemTileMTOpSent(getClock()->currentCycle(), getID(), destMemTile, mes->getAddress());
				log_sched_outgoing();
            }
			
			DEBUG_MSG("\tForwarding to Memory Tile " << destMemTile);
		}
	}
	
	
	void MemoryCPUWrapper::setID(uint16_t new_id) {
		auto config	= root_node->getSimulator()->getSimulationConfiguration()->getUnboundParameterTree();
		uint16_t max_id = config.tryGet("top.arch.params.num_memory_cpus")->getAs<uint16_t>();
		
		sparta_assert(new_id < max_id, "The given ID for the Memory Tile is out of range!");
		
		this->id = new_id;
	}
	
	uint16_t MemoryCPUWrapper::getID() {
		return this->id;
	}
	
	void MemoryCPUWrapper::log_sched_mem_req() {
		uint64_t clk = getClock()->currentCycle();
		if(clk > lastLogTime.sched_mem_req) {
            if(trace_) {
			    logger_->logMemTileOccupancyMC(clk, getID(), sched_mem_req.size());
            }
			lastLogTime.sched_mem_req = clk;
		}
	}
	
	void MemoryCPUWrapper::log_sched_outgoing() {
		uint64_t clk = getClock()->currentCycle();
		if(clk > lastLogTime.sched_outgoing) {
		    logger_->logMemTileOccupancyOutNoC(clk, getID(), sched_outgoing.size());
			lastLogTime.sched_outgoing = clk;
		}
	}
	
	uint64_t MemoryCPUWrapper::getParentAddress(std::shared_ptr<CacheRequest> cr) {
		uint64_t parent_address = 0;
		if(cr->getID() != 0) {
			std::unordered_map<std::uint32_t, Transaction>::iterator transaction_id = transaction_table.find(cr->getID());
			sparta_assert(transaction_id != transaction_table.end(), "Could not find parent instruction!");
			parent_address = transaction_id->second.mcpu_instruction->getAddress();
		}
		return parent_address;
	}
    	
	uint16_t MemoryCPUWrapper::calculateBank(std::shared_ptr<coyote::CacheRequest> r)
	{	
        	uint16_t destination=0;
        
	        if(bank_bits>0) //If more than one bank
	        {
	            uint8_t left=tag_bits;
	            uint8_t right=block_offset_bits;
	            switch(llc_policy)
	            {
	                case CacheDataMappingPolicy::SET_INTERLEAVING:
	                    left+=set_bits-bank_bits;
	                    break;
	                case CacheDataMappingPolicy::PAGE_TO_BANK:
	                    right+=set_bits-bank_bits;
	                    break;
	            }
	            destination=(r->getAddress() << left) >> (left+right);
	        }
	        return destination;
	}
	
	void MemoryCPUWrapper::setLLCInfo(uint64_t size_kbs, uint8_t assoc)
	{
		block_offset_bits=(uint8_t)(ceil(log2(line_size)));
		bank_bits=(uint8_t)ceil(log2(llc_banks));

		uint64_t s=size_kbs*1024;
		uint64_t num_sets=s/(assoc*line_size);
		set_bits=(uint8_t)ceil(log2(num_sets));
		tag_bits=64-(set_bits+block_offset_bits);
	}
}



// vim: set tabstop=4:softtabstop=0:expandtab:shiftwidth=4:smarttab:

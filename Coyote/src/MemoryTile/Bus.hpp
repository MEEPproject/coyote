#ifndef __BUS_HPP__
#define __BUS_HPP__

#include <queue>
#include "sparta/events/UniqueEvent.hpp"


namespace coyote {
	template <typename T> class Bus {
		public:
			//-- delete standard constructors
			Bus() = delete;
			Bus(Bus const&) = delete;
			Bus& operator=(Bus const&) = delete;
			
			/*!
			 * \brief The bus system accepts elements from multiple sources and stores them in a queue.
			 * To schedule the queue, the Sparta scheduler is used.
			 * @controller_cycle_event: A pointer to the the scheduler for this queue
			 */
			Bus(sparta::UniqueEvent<sparta::SchedulingPhase::Tick> *controller_cycle_event, const uint64_t latency_):latency(latency_) {
				this->controller_cycle_event = controller_cycle_event;
				idle = true;
			}
			
			~Bus() {
				std::queue<T>().swap(bus);	//-- create a new and empty queue and swap it with the queue "bus".
			}
			
			/*!
			 * \brief Peeks into the oldest element in the queue without removing it.
			 * \throw Out of Range Exception: There are no elements in the queue.
			 * \return the oldest element in the queue
			 */
			T front();
			
			/*!
			 * \brief Removes the oldest element in the queue
			 * \throw Out of Range Exception: There are no elements in the queue.
			 */
			void pop();
			
			/*!
			 * \brief Adds a new element to the beginning of the queue.
			 * \param element: The element to be added.
			 */
			void push(T element);
			
			/*!
			 * \brief Reschedule the controller_cycle depending on the configured
			 * latency.
			 */
			void reschedule();
			
			/*!
			 * \brief Return the current occupancy level of the queue
			 * \return The current number of elements in the queue
			 */
			uint32_t size();
						
		private:
			sparta::UniqueEvent<sparta::SchedulingPhase::Tick> *controller_cycle_event;
			std::queue<T> bus;
			bool idle;
			const uint64_t latency;
	};
	
	
	
	
	template <typename T> T Bus<T>::front() {
		sparta_assert(!bus.empty(), "Bus<T>::front: There is no element in the queue.\n");
		return bus.front();
	}

	template <typename T> void Bus<T>::pop() {
		sparta_assert(!bus.empty(), "Bus<T>::pop: No element in the queue to be removed.\n");
		
		bus.pop();
		reschedule();
	}

	template <typename T> void Bus<T>::push(T element) {
		bus.push(element);
		
		if(idle) {
			idle = false;
			controller_cycle_event->schedule(sparta::Clock::Cycle(latency));
		}
	}
	
	template <typename T> void Bus<T>::reschedule() {
		if(bus.empty()) {
			idle = true;
		} else {
			controller_cycle_event->schedule(sparta::Clock::Cycle(latency));
		}
	}
	
	template <typename T> uint32_t Bus<T>::size() {
		return bus.size();
	}

	
	
	
	/*!
	 * \brief A bus class which has an additional queue to "park" elements
	 * temporarily
	 */
	template <typename T> class BusDelay : public Bus<T> {
		public:
			//-- delete standard constructors
			BusDelay() = delete;
			BusDelay(BusDelay const&) = delete;
			BusDelay& operator=(BusDelay const&) = delete;
			
			/**
			 * The bus system accepts elements from multiple sources and stores them in a queue.
			 * To schedule the queue, the Sparta scheduler is used.
			 * @controller_cycle_event: A pointer to the the scheduler for this queue
			 */
			BusDelay(sparta::UniqueEvent<sparta::SchedulingPhase::Tick> *controller_cycle_event, const uint64_t latency_): Bus<T>(controller_cycle_event, latency_) { }
						
			~BusDelay() {
				std::queue<T> empty;
				for(uint8_t i=0; i<32; ++i) {
					empty.swap(delay[i]);	//-- create a new and empty queue and swap it with the queue "bus".
				}
			}
			
			
			void notify(uint8_t reg);
			void add_delay_queue(T element, uint8_t reg);
			
						
		private:
			std::queue<T> delay[32];
	};
	
	
	
	template <typename T> void BusDelay<T>::notify(uint8_t reg) {
		while(!delay[reg].empty()) {
			Bus<T>::push(delay[reg].front());
			delay[reg].pop();
		}
	}
	
	template <typename T> void BusDelay<T>::add_delay_queue(T element, uint8_t reg) {
		delay[reg].push(element);
	}
}

#endif
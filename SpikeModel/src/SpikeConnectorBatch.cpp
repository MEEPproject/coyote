
namespace spike_model
{
    SpikeConnectorBatch::SpikeConnectorBatch() : SpikeConnector::SpikeConnector()
    {
    }

    SpikeConnectorBatch::SpikeConnectorBatch(uint32_t num_cores, unsigned int queue_size) : SpikeConnector::SpikeConnector(num_cores, queue_size)
    {
    }
    
    bool SpikeConnectorBatch::pushInstruction(std::shared_ptr<BaseInstruction> inst, unsigned int core)
    {
        
    }

    std::shared_ptr<BaseInstruction> SpikeConnectorBatch::getInstruction(int core)
    
    bool SpikeConnectorBatch::canWrite(unsigned int core, unsigned int steps)

    void SpikeConnectorBatch::stopSpike(unsigned int with_step)

    void SpikeConnectorBatch::notifyCompletion(unsigned int core)
}

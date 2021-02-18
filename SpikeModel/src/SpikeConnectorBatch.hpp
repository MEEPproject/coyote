#ifndef __SPIKE_CONNECTOR_BATCH_H__
#define __SPIKE_CONNECTORi_BATCH_H__

#include "SpikeConnector.hpp"

namespace spike_model
{
    class SpikeConnectorBatch : public SpikeConnector
    {
        public:

            SpikeConnectorBatch();

            SpikeConnectorBatch(uint32_t num_cores, unsigned int queue_size=1000);
            
            ~SpikeConnectorBatch(){};

            bool pushInstruction(std::shared_ptr<BaseInstruction> inst, unsigned int core) override;

            std::shared_ptr<BaseInstruction> getInstruction(int core) override;
            
            bool canWrite(unsigned int core, unsigned int steps) override;

            void stopSpike(unsigned int with_step) override;

            // Copy assignment operator
            // The thread does not get copied, as Spike does not need access to it.
            /*SpikeConnector  &operator=(const SpikeConnector & rhs)
            {
                core_queues=rhs.core_queues;
                max_queue_size=rhs.max_queue_size;
                dummy_connector=rhs.dummy_connector;

                return *this;
            }*/

            void notifyCompletion(unsigned int core)override;
    };
}
#endif

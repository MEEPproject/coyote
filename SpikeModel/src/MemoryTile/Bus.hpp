#ifndef __BUS_HPP__
#define __BUS_HPP__

#include <queue>
#include "sparta/events/UniqueEvent.hpp"


namespace spike_model {
	template <class T> class Bus {
		public:
			//-- delete standard constructors
			Bus() = delete;
			Bus(Bus const&) = delete;
			Bus& operator=(Bus const&) = delete;
			
			/**
			 * The bus system accepts elements from multiple sources and stores them in a queue.
			 * To schedule the queue, the Sparta scheduler is used.
			 * @controller_cycle_event: A pointer to the the scheduler for this queue
			 */
			Bus(sparta::UniqueEvent<sparta::SchedulingPhase::Tick> *controller_cycle_event) {
				this->controller_cycle_event = controller_cycle_event;
				idle = true;
			}
			
			~Bus() {
				std::queue<T>().swap(bus);	//-- create a new and empty queue and swap it with the queue "bus".
			}
			
			/**
			 * Peeks into the oldest element in the queue without removing it.
			 * \throw Out of Range Exception: There are no elements in the queue.
			 * \return the oldest element in the queue
			 */
			T front();
			
			/**
			 * Removes the oldest element in the queue
			 * \throw Out of Range Exception: There are no elements in the queue.
			 */
			void pop();
			
			/**
			 * Adds a new element to the beginning of the queue.
			 * \param element: The element to be added.
			 */
			void push(T element);
						
		private:
			sparta::UniqueEvent<sparta::SchedulingPhase::Tick> *controller_cycle_event;
			std::queue<T> bus;
			bool idle;
	};
	
	
	
	
	template <class T> T Bus<T>::front() {
		sparta_assert(!bus.empty(), "Bus<T>::front: There is no element in the queue.\n");
		return bus.front();
	}

	template <class T> void Bus<T>::pop() {
		sparta_assert(!bus.empty(), "Bus<T>::pop: No element in the queue to be removed.\n");
		
		bus.pop();
		
		if(bus.empty()) {
			idle = true;
		} else {
			controller_cycle_event->schedule(1);
		}
	}

	template <class T> void Bus<T>::push(T element) {
		bus.push(element);
		
		if(idle) {
			idle = false;
			controller_cycle_event->schedule(1);
		}
	}
}

#endif
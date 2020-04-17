#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <future>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    
    // Perform modification under the lock
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock, [this] { return !_queue.empty(); }); // Pass unique lock to condition variable

    // Remove element from queue
    T msg = std::move(_queue.front());
    _queue.pop_front();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // Perform modification under the lock
    std::lock_guard<std::mutex> lock(_mutex);
    // Add message to queue
    _queue.emplace_back(msg);
    // notify client after adding new mesage into queue
    _condition.notify_one();
}


/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto msg = _queue.receive();
        if(msg == green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // Generate a cycle duration - a random value between 4 and 6 seconds.
    std::random_device rndm;
    std::mt19937 gen(rndm());
    std::uniform_int_distribution<int> dis(4000,6000);
    int cycle_duration = dis(gen);

    auto last_switch_time = std::chrono::system_clock::now();
    // start an infinite loop
    while (true)
    {
        // sleep for 1 ms to reduce processor overhead
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        //get the time since last switch happened
        auto temp_secs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_switch_time);
        int time_since_last_switch = temp_secs.count();

        // perform a switch if last switch happened is greater than random generated cycle duration
        if(time_since_last_switch >= cycle_duration){
            _currentPhase = _currentPhase== red? green: red;

            // send an update to MessageQueue using move semantics
            auto futr = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_queue, std::move(_currentPhase));

            // wait until the future is ready
            futr.wait();

            //update last switch time and reset cycle duration
            last_switch_time = std::chrono::system_clock::now();
            cycle_duration = dis(gen);
        }
    }
    
}


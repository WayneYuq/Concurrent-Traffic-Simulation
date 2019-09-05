#include <iostream>
#include <random>
#include "TrafficLight.h"

template <typename T>
T MessageQueue<T>::receive()
{
    std::unique_lock<std::mutex> uLock(_mtx);
    _cond.wait(uLock, [this] { return !_queue.empty(); });

    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    std::lock_guard<std::mutex> lck(_mtx);
    std::cout << "  Message " << msg << " has been send to the queue." << "\n";

    _queue.push_back(std::move(msg));
    _cond.notify_one();
}



TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    while (true)
    {
        TrafficLightPhase v = _messages.receive();
        std::lock_guard<std::mutex> lck(_mutex);
        if (v == TrafficLightPhase::green)
        {
            std::cout << "  The Light in thread # " << std::this_thread::get_id() << " has turn to green." << "\n";
            break;
        }
    }
    
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    lastUpdate = std::chrono::system_clock::now();
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(4, 6);
    double cycleDuration = 1;
    
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        
        if (timeSinceLastUpdate >= cycleDuration)
        {
            // take a duration by 4-6 seconds.
            std::this_thread::sleep_for(std::chrono::seconds(distr(eng)));
            if (_currentPhase == TrafficLightPhase::red)
            {
                _messages.send(std::move(TrafficLightPhase::green));
                std::lock_guard<std::mutex> lck(_mutex);
                _currentPhase = TrafficLightPhase::green;
            }
            else
            {
                _messages.send(std::move(TrafficLightPhase::red));
                std::lock_guard<std::mutex> lck(_mutex);
                _currentPhase = TrafficLightPhase::red;
            }
            
            lastUpdate = std::chrono::system_clock::now();
        }
    }
    
}

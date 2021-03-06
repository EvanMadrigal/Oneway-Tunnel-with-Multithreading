#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <math.h>
#include "unistd.h"

std::mutex mtx;
int numCarsPassing = 0;                         //Tracks the current amount of cars that are passing the tunnel
bool northLight = true;                         //Where true is Green and False is Red for North
bool southLight = false;
static int m;                                   //Total cars passing north and south
static int n;                                   //Max number of cars that are passing the tunnel, can be less
int i = 1;                                      //Incrementor that displays the cars 1 through m, for easy visualization
std::condition_variable cond;                   //Condition variable that controls the bounder buffer as well as the notify
std::deque<int> Nbuffer;                        // North and South queues that hold the cars temproarily before they are deleted after they pass
std::deque<int> Sbuffer;

class Timer                //Created a custom timer class that allows me to print out accurate running/execution time
{
private:
    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
    std::chrono::time_point<std::chrono::steady_clock> m_CurrTime;
public:
    void Start();
    float GetDuration();
};
void Timer::Start()
{
    m_StartTime = std::chrono::steady_clock::now();
}

float Timer::GetDuration()
{
    m_CurrTime = std::chrono::steady_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_CurrTime - m_StartTime).count();
    return duration/1000;
}

void NBP()
{
 while(m > 0)                                                           //Until max cars are produced
 {
     std::unique_lock<std::mutex> locker(mtx);
     cond.wait(locker, [] (){return Nbuffer.size() < n;});              //Bounded buffer that makes sure the amount of cars that car pass the tunnel does not exceed n
     std::this_thread::sleep_for(std::chrono::seconds(1));              //car arrival rate time for the cars that are waiting to be passed across the tunnel rand()%121 seconds per car would be too long
     Nbuffer.push_back(m);
     //std::cout << "north car arrived..." << std::endl;                //Shows the arrival rate at which the cars are waiting in line before entering the tunnel
     m--;
     locker.unlock();
     cond.notify_all();
 }
}

void SBP()
{
 while(m > 0)
 {
     std::unique_lock<std::mutex> locker(mtx);
     cond.wait(locker, [] (){return Sbuffer.size() < n;});
     std::this_thread::sleep_for(std::chrono::seconds(1));
     Sbuffer.push_back(m);
     //std::cout << "south car arrived..." << std::endl;
     m--;
     locker.unlock();
     cond.notify_all();
 }
}

void consumerN(Timer time)
{
    while(true){
        std::unique_lock<std::mutex> locker(mtx);
        cond.wait(locker,[](){return Nbuffer.size() > 0;});                        //Another buffer check that does not give the lock unless there is cars in the queue waiting
        if(northLight == true)                                                     //Makes sure the light is Green before letting cars pass through
        {
        std::this_thread::sleep_for(std::chrono::seconds(3));                      //Takes 3 seconds for the car to follow the next car
        Nbuffer.pop_back();
        std:: cout<<"North car " << i << " is going through tunnel..." << std::endl;    //Display when the cars are going through the tunnel
        i++;
        numCarsPassing++;
        }
        if(Nbuffer.size() == 0)                                                   //Once there are no more cars left then the lights can switch
        {
            northLight = false;
            southLight = true;
            std::this_thread::sleep_for(std::chrono::seconds(4));                //Takes 60 seconds for the car to pass the bridge
            for(int j = 1; j <= numCarsPassing; j++){
            std::this_thread::sleep_for(std::chrono::seconds(3));                 //3 second delay because the cars where three seconds apart
            std:: cout<<"North car " << j << " of " << numCarsPassing << " passed the tunnel!" << std::endl;    //The displays the cars that are passed and when they made it past the tunnel
            }
            numCarsPassing = 0;
            std::cout << std::endl << "***South light switched to Green and North Light switched to Red***" << std::endl;
            std::cout<< "Time(seconds): " << time.GetDuration() << std::endl;    //displays periodic time after the light has changed
            std::cout << std::endl;
        locker.unlock();
        cond.notify_all();
        }
    }
}

void consumerS(Timer time)
{
    while(true){
        std::unique_lock<std::mutex> locker(mtx);
        cond.wait(locker,[](){return Sbuffer.size() > 0;});
        if(southLight == true){
        std::this_thread::sleep_for(std::chrono::seconds(3));
        Sbuffer.pop_back();
        std:: cout<<"South car " << i << " is going through the tunnel..." << std::endl;
        i++;
        numCarsPassing++;
        }
        if(Sbuffer.size() == 0)
        {
            southLight = false;
            northLight = true;
            std::this_thread::sleep_for(std::chrono::seconds(4));
            for(int j = 1; j <= numCarsPassing; j++){
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std:: cout<<"South car " << j << " of " << numCarsPassing << " passed the tunnel!" << std::endl;
            }
            numCarsPassing = 0;
            std::cout << std::endl <<  "***North Light switched to Green and South Light switched to Red***" << std::endl;
            std::cout<< "Time(seconds): " << time.GetDuration() << std::endl;
            std::cout << std::endl;
            locker.unlock();
            cond.notify_all();
        }
    }
}

int main()
{
    std::cout << "Enter the total cars: ";
    std::cin >> m;
    m--;
    std::cout << std::endl;
    std::cout << "Enter amount of cars that can pass through the bridge: ";
    std::cin >> n;
    n--;
    std::cout << std::endl;
    if(northLight == true)
            std::cout << "***North Light is Green and South Light is Red***" << std::endl <<std::endl;
    else
            std::cout << "South light is Green and North Light is Red" << std::endl;
    Timer time;
    time.Start();
    std::thread NBProducer(NBP);                             //Thread that produces North Bound cars
    std::cout<< "North Bound Thread Producer Created!" << std::endl;
    std::thread SBProducer(SBP);                             //Thread that produces South Bound cars
    std::cout<< "South Bound Thread Producer Created!" << std::endl;
    std::thread conN(consumerN,time);                        //Thread that consumes and controls North bound cars that are passing
    std::cout<< "North Bound Thread Consumer Created!" << std::endl;
    std::thread conS(consumerS,time);                        //Thread that consumes and controls North bound cars that are passing
    std::cout<< "South Bound Thread Consumer Created!" << std::endl;
    std::cout << std::endl;
    
    NBProducer.join();
    SBProducer.join();
    conN.join();
    conS.join();

    return 0;
}


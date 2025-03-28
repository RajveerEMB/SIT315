#include <iostream>
#include <queue>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <fstream>

#define PRODUCER_COUNT 1  
#define CONSUMER_COUNT 3  
#define TOP_SIGNALS 3  

std::queue<std::tuple<int, int, int>> trafficQueue;  
std::unordered_map<int, int> congestionData;  
std::mutex queueLock, dataLock;  
std::condition_variable queueNotifier;  
bool isRunning = true;  

// Function to read traffic data from file and add it to queue
void readTrafficData() {
    std::ifstream dataFile("traffic_info.txt");
    if (!dataFile) {
        std::cerr << "Error: Unable to open traffic_info.txt\n";
        return;
    }

    int timestamp, signalID, carCount;
    while (dataFile >> timestamp >> signalID >> carCount) {
        {
            std::lock_guard<std::mutex> lock(queueLock);
            trafficQueue.push({timestamp, signalID, carCount});
        }
        queueNotifier.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    dataFile.close();
}

// Function to process traffic data
void processTrafficData(int consumerID) {
    while (isRunning || !trafficQueue.empty()) {
        std::unique_lock<std::mutex> lock(queueLock);
        queueNotifier.wait(lock, [] { return !trafficQueue.empty() || !isRunning; });

        while (!trafficQueue.empty()) {
            auto [timestamp, signalID, carCount] = trafficQueue.front();
            trafficQueue.pop();
            lock.unlock();

            {
                std::lock_guard<std::mutex> dataMutex(dataLock);
                congestionData[signalID] += carCount;
            }

            std::cout << "[Monitor " << consumerID << "] Analyzed: Signal " 
                      << signalID << ", Cars Passed: " << carCount << std::endl;

            lock.lock();
        }
    }
}

// Function to display the top congested traffic signals
void displayCongestionReport() {
    std::this_thread::sleep_for(std::chrono::seconds(5));  

    std::lock_guard<std::mutex> lock(dataLock);
    if (congestionData.empty()) {
        std::cout << "No congestion data available yet...\n";
        return;
    }

    std::vector<std::pair<int, int>> sortedData(congestionData.begin(), congestionData.end());
    std::sort(sortedData.begin(), sortedData.end(), [](auto &a, auto &b) {
        return a.second > b.second;  
    });

    std::cout << "\n Top " << TOP_SIGNALS << " Congested Signals:\n";
    for (int i = 0; i < std::min(TOP_SIGNALS, (int)sortedData.size()); i++) {
        std::cout << "Signal " << sortedData[i].first << " - Cars Passed: " << sortedData[i].second << "\n";
    }
}

int main() {
    std::vector<std::thread> producers, consumers;

    // Start the data reading thread
    producers.emplace_back(readTrafficData);

    // Start consumer threads
    for (int i = 0; i < CONSUMER_COUNT; i++) {
        consumers.emplace_back(processTrafficData, i);
    }

    // Wait for the producer to complete
    for (auto &t : producers) {
        t.join();
    }

    // Allow consumers to process remaining data
    isRunning = false;
    queueNotifier.notify_all();

    for (auto &t : consumers) {
        t.join();
    }

    // Generate congestion report
    displayCongestionReport();

    std::cout << "Traffic monitoring completed.\n";
    return 0;
}

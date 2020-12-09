#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>
#include <future>

namespace jp {
    class Timer {
    public:
        Timer(uint64_t millis, std::function<void(void*)> func, void* opaque) : func(func), opaque(opaque) {
            interval = millis;
        }

        ~Timer() {
            running = false;
            if (timer_thread.joinable()) timer_thread.join();
        }

        void start() noexcept {
            static void* d = opaque;
            
            paused = false;

            if (!running) {
                running = true;
                if (timer_thread.joinable()) timer_thread.detach();
                timer_thread = std::thread([&]() {
                    do {
                        if (paused) {
                            std::unique_lock<std::mutex> lock(mutex);
                            condition.wait(lock);
                        }
                        
                        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                        duration_millis += interval;
                        
                        int* data_arr = (int*)d;
                        
                        std::async(std::launch::async, func, d);
                    }
                    while (running);
                    fprintf(stderr, "Timer finished!\n");
                });
            } else {
                if (paused) {
                    paused = false;
                    condition.notify_one();
                }
            }
        }

        void stop() noexcept {
            running = false;
        }
        
        void pause() {
            paused = true;
        }

        void reset() noexcept {
            stop();
            duration_millis = 0;
        }
        
        void set_interval(uint64_t interval_value) { interval = interval_value; }
        
        uint64_t get_interval() { return interval; }

        bool is_running() { return running; }
        
        /// Returns how many milliseconds this timer has run for
        uint64_t get_duration() { return duration_millis; }
    private:
        std::function<void(void*)> func;
        void* opaque;
        std::thread timer_thread;
        std::atomic<uint64_t> duration_millis{0};
        std::atomic<uint64_t> interval{};
        std::atomic_bool running{false};
        std::atomic_bool paused{true};
        std::condition_variable condition;
        std::mutex mutex;

    };
}


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
        Timer(uint64_t millis) {
            interval = millis;
        }

        ~Timer() {
            running = false;
            if (timer_thread.joinable()) timer_thread.join();
        }

        void start(std::function<void(void*)> func, void* opaque) noexcept {
            if (running) return;

            static void* d = opaque;

            timer_thread = std::thread([&]() {
                do {
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                    duration_millis += interval;

                    int* data_arr = (int*)d;

                    std::async(std::launch::async, func, d);
                }
                while (running);
            });

            running = true;
        }

        void stop() noexcept {
            running = false;
        }

        void reset() noexcept {
            stop();
            duration_millis = 0;
        }

        bool is_running() { return running; }
        
        /// Returns how many milliseconds this timer has run for
        uint64_t get_duration() { return duration_millis; }
    private:
        std::thread timer_thread;
        std::atomic<uint64_t> duration_millis{0};
        std::atomic<uint64_t> interval{};
        std::atomic_bool running{false};

    };
}


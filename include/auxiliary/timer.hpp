#pragma once

#include "main_menu/runtime_parameters.hpp"

#include <ctime>
#include <chrono>

class timer {
    private:
        bool time_enabled;
        unsigned long int t_init;
        unsigned long int t_export = 0;
        unsigned long int t_end    = 0;
        unsigned long int elapsed  = 0;

    public:
        timer(time_mode mode)
            : time_enabled(mode != time_mode::Disabled) {}

        inline void start() {
            t_init = time_enabled ? time(0) : 0;
        }

        inline void interrupt() {
            t_end  = time_enabled ? time(0) : 0;
        }

        inline void resume() {
            const unsigned long int t_export_end = time_enabled ? time(0) : 0;
            t_export += t_export_end - t_end;
        }

        inline void stop() {
            t_end = ((not time_enabled) || t_end) ? t_end : time(0);
            elapsed = t_end - t_init - t_export;
        }

        void print() const {
            if (not time_enabled)
                return;
            if (elapsed < 60)
                printf("Total duration: %lu seconds\n", elapsed);
            else if (elapsed < 3600)
                printf("Total duration: %lu minutes %lu seconds\n", elapsed / 60, elapsed % 60);
            else
                printf("Total duration: %lu hours %lu minutes %lu seconds\n", elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
        }
};

class timer_ms {
    private:

        enum class timer_state {
            Stopped, Running  
        };

        uint64_t time_start = 0;
        uint64_t time_end   = 0;
        timer_state state;

    public:

        /* Returns the current time in milliseconds */
        static uint64_t get_time() {
            return duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
        }

        using enum timer_state;

        void start() {
            time_start = get_time();
            state = Running;
        }

        void step() {
            if (state == Stopped)
                throw;
            time_end = get_time();
        }

        void stop() {
            step();
            state = Stopped;
        }

        uint64_t elapsed() const {
            if (state == Running)
                throw;
            return time_end - time_start;
        }

        void print() const {
            printf("Time: %llums\n", elapsed());
        }        
};
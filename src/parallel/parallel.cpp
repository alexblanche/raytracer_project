#include "parallel/parallel.hpp"
#include "parameters.hpp"

#include <thread>
#include <vector>
#include <iostream>

[[maybe_unused]]
static void parallel_for_aux(const unsigned int nb_elements,
    const std::function<void (int start, int end)>& functor,
    int wanted_nb_threads = 0) {
    
    const unsigned int nb_threads_hint = std::thread::hardware_concurrency();
    
    unsigned int nb_threads = wanted_nb_threads != 0 ? wanted_nb_threads : 8;
    if (nb_threads_hint != 0)
        nb_threads = std::min(nb_threads_hint, nb_threads);

    const unsigned int batch_size = nb_elements / nb_threads;
    
    std::vector<std::thread> pool;
    pool.reserve(nb_threads);

    for (unsigned int i = 0; i < nb_threads; i++) {
        const int start = i * batch_size;
        pool.emplace_back(functor, start, start + batch_size);
    }
    
    // Remainder
    functor(nb_threads * batch_size, nb_elements);

    for (std::thread& thread : pool) {
        thread.join();
    }
}

void parallel_for(int nb_elements, const std::function<void (int i)>& functor) {

    if constexpr (PARALLELISM == parallelism::Enabled) {
        parallel_for_aux(nb_elements, [&functor] (int start, int end) {
            for (int i = start; i < end; i++) {
                functor(i);
            }
        });
    }
    else {
        for (int i = 0; i < nb_elements; i++)
            functor(i);
    }
}

void parallel_for(int nb_elements, const std::function<void (int start, int end)>& functor, int nb_threads) {

    if constexpr (PARALLELISM == parallelism::Enabled) {
        parallel_for_aux(nb_elements, functor, nb_threads);
    }
    else {
        functor(0, nb_elements);
    }
}
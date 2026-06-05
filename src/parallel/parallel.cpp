#include "parallel/parallel.hpp"

#include <thread>
#include <vector>
#include <iostream>

static void parallel_for_aux(const unsigned int nb_elements,
                  const std::function<void (int start, int end)>& functor) {
    
    const unsigned int nb_threads_hint = std::thread::hardware_concurrency();
    const unsigned int nb_threads = nb_threads_hint != 0 ? nb_threads_hint : 8;

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
    parallel_for_aux(nb_elements, [&functor] (int start, int end) {
        for (int i = start; i < end; i++) {
            functor(i);
        }
    });
}

void parallel_for(int nb_elements, const std::function<void (int start, int end)>& functor) {
    parallel_for_aux(nb_elements, functor);
}
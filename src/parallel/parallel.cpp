#include "parallel/parallel.hpp"

#include <thread>
#include <vector>

void parallel_for(const unsigned int nb_elements,
                  std::function<void (int start, int end)> functor) {
    
    const unsigned int nb_threads_hint = std::thread::hardware_concurrency();
    const unsigned int nb_threads = nb_threads_hint != 0 ? nb_threads_hint : 8;

    const unsigned int batch_size       = nb_elements / nb_threads;
    const unsigned int batch_remainder  = nb_elements % nb_threads;

    std::vector<std::thread> my_threads;
    my_threads.reserve(nb_threads);

    // printf("Parallel execution: %u threads, batch size = %u\n", nb_threads, batch_size);

    for(unsigned int i = 0; i < nb_threads; i++) {
        const int start = i * batch_size;
        my_threads.emplace_back(functor, start, start + batch_size);
    }
    
    // Deform the elements left
    if (batch_remainder != 0) {
        const int start = nb_threads * batch_size;
        functor(start, start + batch_remainder);
    }

    // Wait for the other thread to finish their task
    for (std::thread& thread : my_threads) {
        thread.join();
    }
}
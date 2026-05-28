#include "parallel/parallel.hpp"

#include <thread>
#include <vector>

static void parallel_for_aux(const unsigned int nb_elements,
                  const std::function<void (int start, int end)>& functor) {
    
    const unsigned int nb_threads_hint = std::thread::hardware_concurrency();
    const unsigned int nb_threads = nb_threads_hint != 0 ? nb_threads_hint : 8;

    const unsigned int batch_size       = nb_elements / nb_threads;
    const unsigned int batch_remainder  = nb_elements % nb_threads;

    std::vector<std::thread> my_threads;
    my_threads.reserve(nb_threads);

    for (unsigned int i = 0; i < nb_threads; i++) {
        const int start = i * batch_size;
        my_threads.emplace_back(functor, start, start + batch_size);
    }
    
    if (batch_remainder != 0) {
        const int start = nb_threads * batch_size;
        functor(start, start + batch_remainder);
    }

    for (std::thread& thread : my_threads) {
        thread.join();
    }
}

void parallel_for(int nb_elements, const std::function<void (int i)>& functor) {
    parallel_for_aux(nb_elements, [&] (int start, int end) {
        for (int i = start; i < end; i++) {
            functor(i);
        }
    });
}

void parallel_for(int nb_elements, const std::function<void (int start, int end)>& functor) {
    parallel_for_aux(nb_elements, functor);
}
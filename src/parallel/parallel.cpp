#include "parallel/parallel.hpp"
#include "parameters.hpp"

#include <thread>
#include <vector>

static void parallel_for_aux(const int nb_elements,
    const std::function<void (int start, int end)>& functor,
    int wanted_nb_threads = 0) {
    
    const int nb_threads_hint = std::thread::hardware_concurrency();
    if (nb_threads_hint != 0)
        wanted_nb_threads = std::min(wanted_nb_threads, nb_threads_hint);
    const int nb_threads =
        wanted_nb_threads != 0 ?
              wanted_nb_threads
            : (nb_threads_hint != 0) ? nb_threads_hint : 8;

    const int batch_size = nb_elements / nb_threads;
    
    std::vector<std::thread> pool;
    pool.reserve(nb_threads);

    for (int i = 0; i < nb_threads; i++) {
        const int start = i * batch_size;
        pool.emplace_back(functor, start, start + batch_size);
    }
    
    // Remainder
    functor(nb_threads * batch_size, nb_elements);

    for (std::thread& thread : pool) {
        thread.join();
    }
}

void parallel::parallel_for(int nb_elements, const std::function<void (int i)>& functor) {

    if constexpr (PARALLELISM == parallelism::Enabled)
        parallel_for_aux(nb_elements, [&functor] (int start, int end) {
            for (int i = start; i < end; i++)
                functor(i);
        });
    else
        for (int i = 0; i < nb_elements; i++)
            functor(i);
}

void parallel::parallel_for(int nb_elements, const std::function<void (int start, int end)>& functor, int nb_threads) {

    if constexpr (PARALLELISM == parallelism::Enabled)
        parallel_for_aux(nb_elements, functor, nb_threads);
    else
        functor(0, nb_elements);
}

/* Adaptative parallel loop */

using namespace parallel;

/* Greedy algorithm that splits the weights into batch_number contiguous sets */
adaptative::distribution distribute(std::vector<unsigned int> weights, const unsigned int batch_number) {

    int total_weight = 0;
    for (auto w : weights)
        total_weight += w;
    
    int target_batch_weight = total_weight / batch_number;

    adaptative::distribution distr(batch_number + 1);
    distr[0] = 0;

    int i = 0;
    const int bound = weights.size() - 1;

    for (unsigned int current_batch = 0; current_batch < batch_number - 1; current_batch++) {
        
        int batch_weight = 0;
        while (i < bound && batch_weight < target_batch_weight) {
            batch_weight += weights[i];
            i++;
        }
        distr[current_batch + 1] = i + 1;
        target_batch_weight -= (batch_weight - target_batch_weight);
    }
    
    distr[batch_number] = weights.size();
    return distr;
}

void parallel::parallel_for(const adaptative::distribution& distr,
    const std::function<void (int start, int end)>& functor) {
    
    const int nb_threads = adaptative::nb_batches(distr);
    
    std::vector<std::thread> pool;
    pool.reserve(nb_threads);

    for (int i = 0; i < nb_threads; i++) {
        pool.emplace_back(functor, distr[i], distr[i + 1]);
    }

    for (std::thread& thread : pool) {
        thread.join();
    }
}
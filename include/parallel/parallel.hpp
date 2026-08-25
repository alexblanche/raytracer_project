#pragma once

#include <functional>
#include <vector>

namespace parallel {
    void parallel_for(int nb_elements, const std::function<void (int i)>& functor);
    void parallel_for(int nb_elements, const std::function<void (int start, int end)>& functor, int nb_threads = 0);

    class adaptative {

        public:
            using batch_size = int;
            using distribution = std::vector<batch_size>;

            static unsigned int nb_batches(const distribution& distr) {
                return distr.size() - 1;
            }
            
            static distribution distribute(std::vector<unsigned int> weights, unsigned int batch_number);
    };

    void parallel_for(const adaptative::distribution& distr,
        const std::function<void (int start, int end)>& functor);
}
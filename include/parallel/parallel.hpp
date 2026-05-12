#pragma once

#include <functional>

void parallel_for(unsigned int nb_elements,
                  std::function<void (int start, int end)> functor);

#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; i++)
#define PARALLEL_FOR_END() })

#pragma once

#include <functional>

void parallel_for(int nb_elements, const std::function<void (int i)>& functor);

void parallel_for(int nb_elements, const std::function<void (int start, int end)>& functor);
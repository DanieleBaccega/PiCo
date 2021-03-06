/*
 This file is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 PiCo is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 */

/* 
 *******************************************************************************
 *
 * File:         generate_pairs.cpp
 * Description:  generate a text file of random char-int pairs
 * Author:       Maurizio Drocco
 * Language:     C++
 * Created on:   Feb 9, 2016
 *
 *******************************************************************************
 */

#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <cassert>

#include "../../pico/KeyValue.hpp"

#include "../common/utils.hpp"

typedef pico::KeyValue<char, int> KV;

int main(int argc, char** argv)
{
    /* parse command line */
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0];
        std::cerr << " <n. of pairs>\n";
        return -1;
    }
    unsigned long long n_lines = get_size(argv[1]);

    /* generate and emit random lines */
    std::default_random_engine rng;
    std::uniform_int_distribution<char> sym_dist('a', 'a' + 25);
    std::uniform_int_distribution<int> num_dist(-10, 10);
    for (unsigned line_i=0; line_i < n_lines; ++line_i)
    {
    		KV kv(sym_dist(rng), num_dist(rng));
    		std::cout << kv.to_string() << std::endl;
        print_progress((float) (line_i + 1) / n_lines);
    }

    std::cerr << "\ndone\n";

    return 0;
}

/*
 This file is part of PiCo.
 PiCo is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 PiCo is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 You should have received a copy of the GNU Lesser General Public License
 along with PiCo.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * stock_pricing.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: drocco
 */

/*
 * This code implements a pipeline for batch processing of stocks.
 * It first computes a price for each option from a text file,
 * then it extracts the maximum price for each stock name.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include <pico/pico.hpp>

#include "defs.h"
#include "common.hpp"
#include "black_scholes.hpp"

int main(int argc, char** argv) {
	// parse command line
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << " <input file> <output file> \n";
		return -1;
	}
	std::string in_fname(argv[1]), out_fname(argv[2]);

	/*
	 * define a batch pipeline that:
	 * 1. read options from file
	 * 2. computes prices by means of the blackScholes pipeline
	 * 3. extracts the maximum price for each stock name
	 * 4. write prices to file
	 */
	Map<std::string, StockAndPrice> blackScholes([] (const std::string& in) {
		OptionData opt;
		char otype, name[128];
		parse_opt(opt, otype, name, in);
		opt.OptionType = (otype == 'P');
		return StockAndPrice(std::string(name), black_scholes(opt));
	});

	auto stockPricing = Pipe() //
	.add(ReadFromFile(in_fname)) //
	.add(blackScholes) //
	.add(SPReducer()) //
	.add(WriteToDisk<StockAndPrice>(out_fname));

	/* print the semantic graph and generate dot file */
	stockPricing.print_semantics();
	stockPricing.to_dotfile("stock_pricing.dot");

	/* execute the pipeline */
	stockPricing.run();

	/* print execution time */
	std::cout << "done in " << stockPricing.pipe_time() << " ms\n";

	return 0;
}

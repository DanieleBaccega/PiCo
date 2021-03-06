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
 * Global.hpp
 *
 *  Created on: Dec 28, 2016
 *      Author: misale
 *
 *   TODO - user-friendly argc/argv
 */

#ifndef DEFINES_GLOBAL_HPP_
#define DEFINES_GLOBAL_HPP_

#include <getopt.h>
#include <iostream>
#include <string>
#include <sstream>

#include <ff/mapper.hpp>

namespace pico {

struct {
	int MICROBATCH_SIZE = 8;
} global_params;

} /* namespace pico */

#endif /* DEFINES_GLOBAL_HPP_ */

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
 * FarmCollector.hpp
 *
 *  Created on: Dec 9, 2016
 *      Author: misale
 */

#ifndef INTERNALS_FFOPERATORS_FARMCOLLECTOR_HPP_
#define INTERNALS_FFOPERATORS_FARMCOLLECTOR_HPP_

#include "../../Internals/utils.hpp"

using namespace pico;

class ForwardingCollector : public ff::ff_node {
public:
	ForwardingCollector(int nworkers_) :
			nworkers(nworkers_), picoEOSrecv(0), picoSYNCrecv(0) {
	}

	void* svc(void* task) {
		if (task == PICO_EOS) {
			if (++picoEOSrecv == nworkers)
				return task;
			return GO_ON;
		}

		if (task == PICO_SYNC) {
			if (++picoSYNCrecv == nworkers)
				return task;
			return GO_ON;
		}

		return task;
	}

private:
	int nworkers;
	unsigned picoEOSrecv, picoSYNCrecv;
};

#endif /* INTERNALS_FFOPERATORS_FARMCOLLECTOR_HPP_ */

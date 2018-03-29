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

#include "base_nodes.hpp"

class ForwardingCollector: public base_collector {
public:
	using base_collector::base_collector;

	void kernel(base_microbatch* mb) {
		send_mb(mb);
	}
};

/* unpacks and streams token-collectors */
template<typename coll_t>
class UnpackingCollector: public base_collector {
	typedef typename coll_t::cnode cnode_t;
public:
	using base_collector::base_collector;

	void kernel(base_microbatch *mb) {
		auto wmb = reinterpret_cast<mb_wrapped<cnode_t> *>(mb);
		cnode_t *it_, *it = wmb->get();
		/* send out all the micro-batches in the list */
		while (it) {
			send_mb(it->mb);

			/* clean up and skip to the next micro-batch */
			it_ = it;
			it = it->next;
			FREE(it_);
		};
		DELETE(wmb);
	}
};

#endif /* INTERNALS_FFOPERATORS_FARMCOLLECTOR_HPP_ */

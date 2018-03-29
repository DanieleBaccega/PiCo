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
 * PReduceBatch.hpp
 *
 *  Created on: Mar 15, 2018
 *      Author: drocco
 */

#ifndef INTERNALS_FFOPERATORS_PREDUCEBATCH_HPP_
#define INTERNALS_FFOPERATORS_PREDUCEBATCH_HPP_

#include <unordered_map>

#include <ff/farm.hpp>

#include "../../Internals/utils.hpp"
#include "../SupportFFNodes/PReduceCollector.hpp"
#include "../ff_config.hpp"
#include "../../Internals/TimedToken.hpp"
#include "../../Internals/Microbatch.hpp"

using namespace ff;
using namespace pico;

/*
 * todo
 * this approach has poor performance, should be replaced by shuffling
 *
 * the (stateful) reduce-by-key farm updates the internal key-value state
 * and, upon c-stream end, streams out the state
 */
template<typename TokenType>
class RBK_farm: public NonOrderingFarm {
	typedef typename TokenType::datatype Out;
	typedef typename Out::keytype OutK;
	typedef typename Out::valuetype OutV;

public:
	RBK_farm(int red_par, std::function<OutV(OutV&, OutV&)> reducef) {
		auto e = new Emitter(red_par, getlb());
		this->setEmitterF(e);
		auto c = new ForwardingCollector(red_par);
		this->setCollectorF(c);
		std::vector<ff_node *> w;
		for (int i = 0; i < red_par; ++i)
			w.push_back(new Worker(reducef));
		this->add_workers(w);
		this->cleanup_all();
	}

private:
	class Worker: public base_filter {
	public:
		Worker(std::function<OutV(OutV&, OutV&)>& reducef_kernel_) :
				reduce_kernel(reducef_kernel_) {
		}

		void kernel(base_microbatch *in_mb) {
			/*
			 * got a microbatch to process and delete
			 */
			auto in_microbatch = reinterpret_cast<kv_mb*>(in_mb);
			auto tag = in_mb->tag();
			auto &s(tag_state[tag]);

			/* reduce the micro-batch updateing internal state */
			for (Out &kv : *in_microbatch) {
				auto &k(kv.Key());
				if (s.kvmap.find(k) != s.kvmap.end())
					s.kvmap[k] = reduce_kernel(kv.Value(), s.kvmap[k]);
				else
					s.kvmap[k] = kv.Value();
			}

			//clean up
			DELETE(in_mb);
		}

		void cstream_end_callback(base_microbatch::tag_t tag) {
			auto &s(tag_state[tag]);
			auto mb = NEW<kv_mb>(tag, global_params.MICROBATCH_SIZE);
			for (auto it = s.kvmap.begin(); it != s.kvmap.end(); ++it) {
				new (mb->allocate()) Out(it->first, it->second);
				mb->commit();
				if (mb->full()) {
					send_mb(mb);
					mb = NEW<kv_mb>(tag, global_params.MICROBATCH_SIZE);
				}
			}

			/* send out the remainder micro-batch or destroy if spurious */
			if (!mb->empty()) {
				send_mb(mb);
			} else {
				DELETE(mb);
			}
		}

	private:
		typedef Microbatch<TokenType> kv_mb;

		std::function<OutV(OutV&, OutV&)> reduce_kernel;
		struct key_state {
			std::unordered_map<OutK, OutV> kvmap;
		};
		std::unordered_map<base_microbatch::tag_t, key_state> tag_state;
	};

	class Emitter: public bk_emitter_t {
	public:
		Emitter(unsigned nworkers_, typename NonOrderingFarm::lb_t * lb_) :
				bk_emitter_t(lb_, nworkers_), nworkers(nworkers_) {
		}

		void kernel(base_microbatch *in_mb) {
			auto in_microbatch = reinterpret_cast<mb_t *>(in_mb);
			send_mb_to(in_mb, key_to_worker((*in_microbatch->begin()).Key()));
		}

	private:
		typedef typename TokenType::datatype DataType;
		typedef typename DataType::keytype keytype;
		typedef Microbatch<TokenType> mb_t;
		unsigned nworkers;

		inline size_t key_to_worker(const keytype& k) {
			return std::hash<keytype> { }(k) % nworkers;
		}
	};
};

#endif /* INTERNALS_FFOPERATORS_PREDUCEBATCH_HPP_ */

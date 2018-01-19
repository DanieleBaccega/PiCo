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
 * FlatMap.hpp
 *
 *  Created on: Aug 18, 2016
 *      Author: misale
 */

#ifndef OPERATORS_FLATMAP_HPP_
#define OPERATORS_FLATMAP_HPP_

#include "PReduce.hpp"
#include "UnaryOperator.hpp"

#include "../WindowPolicy.hpp"
#include "../Internals/TimedToken.hpp"
#include "../Internals/Token.hpp"

#include "../ff_implementation/OperatorsFFNodes/FMapBatch.hpp"
#include "../ff_implementation/OperatorsFFNodes/FMapPReduceBatch.hpp"
#include "../ff_implementation/SupportFFNodes/FarmWrapper.hpp"

namespace pico {

/**
 * Defines an operator performing a FlatMap, taking in input one element from
 * the input source and producing zero, one or more elements in output.
 * The FlatMap kernel is defined by the user and can be a lambda function, a functor or a function.
 *
 * It implements a data-parallel operator that ignores any kind of grouping or windowing.
 *
 * The kernel is applied independently to all the elements of the collection (either bounded or unbounded).
 */
template <typename In, typename Out>
class FlatMap : public UnaryOperator<In, Out> {
	friend class Pipe;
public:

	/**
	 * \ingroup op-api
	 *
	 * Constructor.
	 * Creates a new FlatMap operator by defining its kernel function  flatMapf: In->Out
	 * @param flatmapf std::function<Out(In)> FlatMap kernel function with input type In producing zero, one or more element of type Out
	 */
	FlatMap(std::function<void(In&, FlatMapCollector<Out> &)> flatmapf_) {
		flatmapf = flatmapf_;
		this->set_input_degree(1);
		this->set_output_degree(1);
        this->set_stype(BOUNDED, true);
        this->set_stype(UNBOUNDED, true);
        this->set_stype(ORDERED, true);
        this->set_stype(UNORDERED, true);
	}

	/**
	 * Returns the name of the operator, consisting in the name of the class.
	 */
	std::string name_short(){
		return "FlatMap";
	}

protected:
	FlatMap<In, Out>* clone() {
		return new FlatMap<In, Out>(flatmapf);
	}

	void run(In* task) {
		assert(false);
#ifdef DEBUG
		std::cerr << "[FLATMAP] running... \n";
#endif
	}

	const OpClass operator_class(){
		return OpClass::FMAP;
	}

	ff::ff_node* node_operator(int parallelism, Operator*) {
		WindowPolicy* win;
		if(this->data_stype()  == StructureType::STREAM){
			win = new BatchWindow<Token<In>>();
			return new FMapBatch<In, Out, ff_ofarm, Token<In>, Token<Out>>(parallelism, flatmapf, win);
		}
		win = new noWindow<Token<In>>();
		return new FMapBatch<In, Out, FarmWrapper, Token<In>, Token<Out>>(parallelism, flatmapf, win);
	}

	ff::ff_node *opt_node(int pardeg, PEGOptimization_t opt, opt_args_t a) {
		assert(opt == FMAP_PREDUCE);
		using t = FMapPReduceBatch<In, Out, FarmWrapper, Token<In>, Token<Out>>;
		auto win = new noWindow<Token<In>>();
		auto nextop = dynamic_cast<PReduce<Out>*>(a.op);
		return new t(pardeg, flatmapf, nextop->kernel(), win);
	}


private:
	std::function<void(In&, FlatMapCollector<Out> &)> flatmapf;
};

} /* namespace pico */

#endif /* OPERATORS_FLATMAP_HPP_ */

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
 * pagerank.cpp
 *
 *  Created on: Mar 12, 2018
 *      Author: drocco
 */

#include <pico/pico.hpp>
using namespace pico;

typedef unsigned Node;
typedef float Rank;
typedef KeyValue<Node, Rank> NRank;
typedef KeyValue<Node, std::vector<Node>> NLinks;

constexpr float DAMPENING = 0.85;
constexpr unsigned niters = 10;
unsigned long long vertices = 0;

int main(int argc, char** argv) {
	// parse command line
	if (argc < 4) {
		printf("Usage: %s <pages file> <links file> <output file>\n", argv[0]);
		return -1;
	}
	std::string in_pages_fname(argv[1]), in_links_fname(argv[2]);
	std::string out_fname(argv[3]);

	/* count vertices */
	std::ifstream in_pages(in_pages_fname);
	std::string in_buffer;
	while(!in_pages.eof()) {
		getline(in_pages, in_buffer);
		++vertices;
	}

	// Map operator parsing lines into node-links pairs
	Map<std::string, NLinks> parseEdges { //
	[] (const std::string &adj) {
		Node src;
		std::vector<Node> links {1};
		sscanf(adj.c_str(), "%u %u", &src, &links[0]);
		return NLinks {src, links};
	} };

	// Reduce-by-key operator converting edge list to adjacency list
	ReduceByKey<NLinks> edgesToAdj { //
	[] (const std::vector<Node> &adj1, const std::vector<Node> &adj2) {
		std::vector<Node> res(adj1);
		res.insert(res.end(), adj2.begin(), adj2.end());
		return res;
	} };

	// Map operator generating initial ranks for node-links
	Map<std::string, NRank> parseAndInitRanks { //
	[] (const std::string &nl) {
		Node n;
		sscanf(nl.c_str(), "%u", &n);
		return NRank {n, 1.0};
	} };

	// By-key join + FlatMap operator, computing ranking updates
	JoinFlatMapByKey<NRank, NLinks, NRank> computeContribs { //
	[] (const NRank &nr, const NLinks &nl, FlatMapCollector<NRank> &c) {
		for( auto &dest: nl.Value() )
		c.add( NRank {dest, nr.Value() / nl.Value().size()});
	} };

	// By-key reduce summing up contributions
	ReduceByKey<NRank> sumContribs { //
	[] (Rank r1, Rank r2) {
		return r1 + r2;
	} };

	// Map operator normalizing node-rank pairs
	Map<NRank, NRank> normalize { //
	[] (const NRank &nr) {
		float jump = (1 - DAMPENING) / vertices;
		return NRank(nr.Key(), nr.Value() * DAMPENING + jump);
	} };

	// Output operator writing the result to file
	WriteToDisk<NRank> writeRanks { out_fname, //
			[] (const NRank &in) {
				return in.to_string();
			} };

	Pipe generateInitialRanks = //
			Pipe { } //
	.add(ReadFromFile(in_pages_fname))
			.add(parseAndInitRanks);

	// The pipe for building the graph to be processed.
	Pipe generateLinks = //
			Pipe { } //
			.add(ReadFromFile(in_links_fname)) //
			.add(parseEdges) //
			.add(edgesToAdj);

	// The pipe that gets iterated to improve the computed ranks
	Pipe improveRanks = //
			Pipe { } //
			.pair_with(generateLinks, computeContribs) //
			.add(sumContribs) //
			.add(normalize);

	// The whole pageRank pipe.
	Pipe pageRank = Pipe { } //
			.to(generateInitialRanks) //
			.to(improveRanks.iterate(FixedIterations { niters })) //
			.add(writeRanks);

	pageRank.run(run_mode::FORCE_NONBLOCKING);

	return 0;
}

/*
 * MultiRate.cpp
 *
 *  Created on: Apr 3, 2019
 *      Author: mirco
 */
#include "MultiRate/MultiRateTaskset.h"
#include "MultiRate/MultiNode.h"

#include <vector>
#include <iostream>

MultiRateTaskset::MultiRateTaskset() :
		dummyNodes_(std::make_shared<DummyNodes>()), hyperPeriod_(0)
{
}

std::shared_ptr<MultiNode>
MultiRateTaskset::addTask(unsigned period, unsigned wcet, unsigned deadline,
		const std::string& name)
{
	auto mult = std::make_shared<MultiNode>();
	mult->period = period;
	mult->wcet = wcet;
	mult->deadline = deadline;
	mult->id = nodes_.size();

	if (name.empty())
		mult->name = std::to_string(mult->id);
	else
		mult->name = name;

	nodes_.push_back(mult);

	if (period > hyperPeriod_)
		hyperPeriod_ = period;

	return mult;
}

std::shared_ptr<MultiNode>
MultiRateTaskset::addTask(unsigned period, unsigned wcet, const std::string& name)
{
	return addTask(period, wcet, period, name);
}

std::shared_ptr<MultiEdge>
MultiRateTaskset::addPrecedenceEdge(std::shared_ptr<MultiNode> from, std::shared_ptr<MultiNode> to)
{
	auto edge = std::make_shared<MultiEdge>();
	edge->from = from;
	edge->to = to;
	edge->dependency = MultiEdge::Dependency::PRECEDENCE;
	edge->jitter = 0; //Precedence does not have jitter
	edges_.push_back(edge);

	return edge;
}

std::shared_ptr<MultiEdge>
MultiRateTaskset::addDataEdge(std::shared_ptr<MultiNode> from, std::shared_ptr<MultiNode> to,
		unsigned jitter)
{
	auto edge = std::make_shared<MultiEdge>();
	edge->from = from;
	edge->to = to;
	edge->dependency = MultiEdge::Dependency::DATA;
	edge->jitter = jitter;
	edges_.push_back(edge);

	return edge;
}

const DAG&
MultiRateTaskset::createBaselineDAG()
{
	baselineDAG_.setPeriod(hyperPeriod_);
	auto start = baselineDAG_.getStart();
	auto end = baselineDAG_.getEnd();
	for (auto& node : nodes_)
	{
		auto nodes = node->createNodes(hyperPeriod_);
		std::vector<Edge> edges;
		edges.push_back(Edge(start, nodes.front()));
		for (unsigned k = 0; k < nodes.size() - 1; k++)
		{
			edges.push_back(Edge(nodes[k], nodes[k + 1]));
		}
		edges.push_back(Edge(nodes.back(), end));
		baselineDAG_.addNodes(nodes);
		baselineDAG_.addEdges(edges);
	}

	dummyNodes_->addToDAG(baselineDAG_);
	// Set Unique IDs
	auto nodes = baselineDAG_.getNodes();
	for (unsigned k = 0; k < nodes.size(); k++)
		nodes[k]->uniqueId = k;
	return baselineDAG_;
}

std::vector<DAG>
MultiRateTaskset::createDAGs()
{
	std::vector<std::vector<std::vector<Edge>>> edgeSets;

	std::vector<int> permutSets;
	for (auto edge : edges_)
	{
		edgeSets.push_back(edge->translateToEdges());
		permutSets.push_back(edgeSets.back().size());
	}
	permutSets.push_back(1);

	std::vector<int> permutation(edgeSets.size(), 0);
	int numPermutations = 1;
	for (const auto& it : edgeSets)
		numPermutations *= it.size();

	for (int k = permutSets.size() - 2; k >= 0; k--)
	{
		permutSets[k] = permutSets[k + 1] * permutSets[k];
	}

	int cyclicDags = 0;
	int brokenDummyChain = 0;

	std::vector<DAG> dags;
	for (int k = 0; k < numPermutations; k++)
	{
		DAG dag(baselineDAG_);

		int tmp = k;
		for (int i = 0; i < permutation.size(); i++)
		{
			permutation[i] = tmp / permutSets[i + 1];
			tmp = tmp % permutSets[i + 1];
		}

		for (int n = 0; n < edgeSets.size(); n++)
		{
			dag.addEdges(edgeSets[n][permutation[n]]);
		}

		if (dag.isCyclic())
		{
			cyclicDags++;
			continue;
		}

		dag.transitiveReduction();

		//Check if Dummy chain was broken, making the DAG not schedulable
		if (dummyNodes_->brokenDummyChain(dag))
		{
			brokenDummyChain++;
			continue;
		}

		dags.push_back(dag);
	}

	std::cout << cyclicDags << " cyclic Dags were excluded" << std::endl;
	std::cout << brokenDummyChain << " Dags were excluded due to broken Dummy Chain" << std::endl;

	return dags;
}

std::shared_ptr<DummyNodes>
MultiRateTaskset::getDummyNodes() const
{
	return dummyNodes_;
}

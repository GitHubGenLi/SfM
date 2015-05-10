#pragma once

#include <unordered_map>

#include "structures.hpp"

/**
 * Associativity of cameras (matching features, essentially camera pairs)
 */
class Associativity
{
private:
	typedef std::pair<int, int> PairIndex;
	typedef std::function<bool (const int, ImagePair* pair)> walk_func;

	std::unordered_map<PairIndex, ImagePair*> _map;

	PairIndex makeIndex(int a, int b) const;

public:
	int n;

	Associativity(): n(0) {}
	Associativity(int _n) : n(_n) {}

	ImagePair*& operator()(const int i, const int j);
	ImagePair* operator()(const int i, const int j) const;

	pImagePairs getAssociatedPairs(const int i);
	void walk(walk_func func);
};

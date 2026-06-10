#pragma once
#include <iostream>
using namespace std;
class Building;
class GoodFriend
{
public:
	Building* building;
	GoodFriend();
	void visit();
};
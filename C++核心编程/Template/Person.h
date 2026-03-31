#pragma once
#include<iostream>
#include<vector>
using namespace std;

template<typename NameType, typename AgeType = int>//默认类型
class Person
{
public:
	Person(NameType name, AgeType age);
	//{
	//	this->name = name;
	//	this->age = age;
	//}
	void ShowInfo();
	NameType name;
	AgeType age;
};

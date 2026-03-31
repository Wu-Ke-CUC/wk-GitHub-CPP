#include"Person.h"
template<typename NameType, typename AgeType>
Person<NameType, AgeType>::Person(NameType name, AgeType age)
{
	this->name = name;
	this->age = age;
}
template<typename NameType, typename AgeType>
void Person<NameType, AgeType>::ShowInfo()
{
	cout << this->name << " " << this->age << endl;
}
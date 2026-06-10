#include<iostream>
using namespace std;

class Person
{
public:
	string name;
	void showInfo()
	{
		cout << name << " " << age << endl;
	}
protected:
	int age;
};
class Student : public Person
{
public:
	string major;
	Student(string name, int age, string major)
	{
		this->name = name;
		this->age = age;
		this->major = major;
	}
	void showMajor()
	{
		cout << major << endl;
	}
};

int main()
{

}
#include<iostream>
using namespace std;

class Person
{
	friend Person operator-(Person& person1, Person& person2);
	friend ostream& operator<<(ostream& cout, Person& person);
	friend Person& operator--(Person& person);
	friend Person operator--(Person& person, int);
	friend bool operator==(Person& person1, Person& person2);
	friend bool operator!=(Person& person1, Person& person2);

private:
	int age;
	int* money;
public:
	Person operator+(Person& person)				//成员函数运算符重载
	{
		Person temp;
		temp.age = age + person.age;
		return temp;
	}
	Person& operator++()							//返回引用是为了一直对能该数据操作前置加加
	{
		age++;
		return *this;
	}
	Person operator++(int)							//后置加加
	{
		Person temp = *this;
		age++;
		return temp;
	}
	Person& operator=(Person& person)
	{
		age = person.age;
		if (money != NULL)
		{
			delete money;
			money = NULL;
		}
		money = new int(*person.money);
		return *this;
	}
	void operator()(int n)							//重载函数(),仿函数
	{
		for (int i = 0; i < n; i++)
		{
			cout << *this << endl;
		}
	}
	Person(){}
	Person(int age,int money)
	{
		this->age = age;
		this->money = new int(money);
	}
};

Person operator-(Person& person1, Person& person2)	//全局函数运算符重载
{
	Person temp;
	temp.age = person1.age - person2.age;
	return temp;
}
ostream& operator<<(ostream& cout,Person& person)
{
	cout << person.age;
	if (person.money != NULL)
	{
		cout << " " << *person.money;
	}
	return cout;
}
Person& operator--(Person& person)
{
	person.age--;
	return person;
}
Person operator--(Person& person, int)
{
	Person temp = person;
	person.age++;
	return temp;
}
bool operator==(Person& person1, Person& person2)
{
	return (person1.age == person2.age && *person1.money == *person2.money);
}
bool operator!=(Person& person1, Person& person2)
{
	return !(person1 == person2);
}

int main()
{
	Person person1(18,20);
	Person person2(16,30);
	Person person3 = person1 + person2;
	cout << person3 << endl;
	Person person4 = person1 - person2;
	cout << person4 << endl;
	++person4;
	cout << person4 << endl;
	person4 = person3 = person2;
	cout << person4 <<" " <<person3<<" "<<person2 << endl;
	cout << (person4 != person3) << endl;

	person4(2);

	return 0;
}
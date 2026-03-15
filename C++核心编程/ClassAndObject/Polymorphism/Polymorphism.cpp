#include<iostream>
#include<string>
using namespace std;

class Animal
{
public:
	virtual void Speak() = 0;	//纯虚函数
	//virtual ~Animal()			//虚析构
	//{
	//	cout << "Animal is deleted" << endl;
	//}
	virtual ~Animal() = 0;		//纯虚析构
};
Animal::~Animal()
{
	cout << "Animal is virtually deleted" << endl;
}
class Cat : public Animal
{
public:
	Cat(string name)
	{
		this->name = new string(name);
	}
	virtual void Speak()
	{
		cout << "Cat is speaking." << endl;
	}
	~Cat()
	{
		if (name != NULL)
		{
			cout << *name << "is deleted" << endl;
			delete name;
			name = NULL;
		}
	}
private:
	string *name;
};
void DoSpeak(Animal& animal)
{
	animal.Speak();
}

int main()
{
	Animal* cat = new Cat("Kitte");
	cat->Speak();
	delete cat;

	return 0;
}
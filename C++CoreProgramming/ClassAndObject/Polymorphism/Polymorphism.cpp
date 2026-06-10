#include<iostream>
#include<string>
using namespace std;

__interface IMove
{
	void Walk();
};

class Animal : public IMove
{
public:
	virtual void Speak() = 0;	//纯虚函数
	virtual void Walk() = 0;
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
	void Speak() override
	{
		cout << "Cat is speaking." << endl;
	}
	void Walk() override
	{
		cout << "Cat is moving" << endl;
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

class A
{
	char a[2];
public:
	virtual void aa(){}
};
class B :public virtual A
{
	char a[2];
	char b[2];
public:
	virtual void aa(){}
	virtual void bb(){}
};
class C :public virtual B
{
	char a[2];
	char b[2];
	char c[2];
public:
	virtual void aa(){}
	virtual void bb(){}
	virtual void cc(){}
};
int main()
{
	Animal* cat = new Cat("Kitte");
	cat->Speak();
	cat->Walk();
	delete cat;

	cout << sizeof(A) << endl << sizeof(B) << endl << sizeof(C);

	return 0;
}
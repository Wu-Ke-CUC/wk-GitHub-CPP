#include"Person.cpp"//方法一
//方法二：将.h文件和.cpp文件写在一起，文件后缀名为.hpp

template<typename T>
void MySwap(T& element1, T& element2) noexcept	//类型要一致
{
	T temp = element1;
	element1 = element2;
	element2 = temp;
}
template<typename T>
void MySort(vector<T>& arr,int l,int r)
{
	if (l >= r)return;
	T base = arr[l];
	int i = l, j = r;
	while (i != j)
	{
		while (i < j && arr[j] >= base)j--;
		if (i < j)arr[i++] = arr[j];
		while (i < j && arr[i] <= base)i++;
		if (i < j)arr[j--] = arr[i];
	}
	arr[i] = base;
	MySort(arr, l, i - 1);
	MySort(arr, i + 1, r);
}
template<typename T>
void PrintArray(vector<T> arr)
{
	for (T num : arr)
	{
		cout << num << " ";
	}
	cout << endl;
}
template<typename T>
T MyAdd(T element1, T element2)
{
	return element1 + element2;
}

//template<typename NameType,typename AgeType = int>//默认类型
//class Person
//{
//public:
//	Person(NameType name, AgeType age);
//	//{
//	//	this->name = name;
//	//	this->age = age;
//	//}
//	void ShowInfo();
//	NameType name;
//	AgeType age;
//};
//template<typename NameType, typename AgeType>
//Person<NameType, AgeType>::Person(NameType name, AgeType age)
//{
//	this->name = name;
//	this->age = age;
//}
//template<typename NameType, typename AgeType>
//void Person<NameType, AgeType>::ShowInfo()
//{
//	cout << this->name << " " << this->age << endl;
//}
template<typename NameType, typename AgeType>
class Student;
template<typename NameType, typename AgeType>
void ShowStudent2(Student<NameType, AgeType> student)
{
	cout << student.name << " " << student.age << endl;
}
template<typename NameType,typename AgeType = int>//默认类型
class Student
{
	//全局函数类内实现
	friend void ShowStudent1(Student<NameType, AgeType> student)
	{
		cout << student.name << " " << student.age << endl;
	}
	//全局函数类外实现
	friend void ShowStudent2<>(Student<NameType, AgeType> student);
public:
	Student(NameType name, AgeType age);
	void ShowInfo();
private:
	NameType name;
	AgeType age;
};
template<typename NameType, typename AgeType>
Student<NameType, AgeType>::Student(NameType name, AgeType age)
{
	this->name = name;
	this->age = age;
}
template<typename NameType, typename AgeType>
void Student<NameType, AgeType>::ShowInfo()
{
	cout << this->name << " " << this->age << endl;
}

class IProduct
{
	virtual void Initilize()
	{

	}
};

template <class T>
class AbstrectFectory
{
private:
	T Creat()
	{

	}
};
class Enemy :IProduct
{
public:
	int hp;
	virtual void Initilize()
	{

	}
};
class Goblin :public Enemy
{
public:
	void Initilize() override
	{

	}
};
class GoblinFactory :public AbstrectFectory<Enemy>
{
public:
	Goblin Creat()
	{

	}
};

int main()
{
	int intNum1 = 10;
	int intNum2 = 20;
	//1，自动类型推导
	//MySwap(intNum1, intNum2);
	//2，显示指定类型
	MySwap<int>(intNum1, intNum2);
	cout << intNum1 << " " << intNum2 << endl;

	float floatNum1 = 3.14f;
	float floatNum2 = 1.41f;
	MySwap(floatNum1, floatNum2);
	cout << floatNum1 << " " << floatNum2 << endl;

	vector<int> intNums = { 5,1,4,2,3 };
	MySort(intNums, 0, intNums.size() - 1);
	PrintArray(intNums);
	vector<char> chars = { 'g','f','d','s','a'};
	MySort(chars, 0, chars.size() - 1);
	PrintArray(chars);

	char a = 55;
	int b = 3;
	cout << MyAdd<char>(a, b) << endl;

	Person<string, int> p = { "zhangsan",18 };
	cout << p.name << " " << p.age << endl;
	p.ShowInfo();

	Student<string, int> s = { "Tom",20 };
	ShowStudent1(s);
	ShowStudent2(s);
	return 0;
}
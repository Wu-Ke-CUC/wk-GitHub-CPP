#include<iostream>
#include<vector>
using namespace std;

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

template<typename NameType,typename AgeType = int>//默认类型
class Person
{
public:
	Person(NameType name, AgeType age)
	{
		this->name = name;
		this->age = age;
	}

	NameType name;
	AgeType age;
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

	return 0;
}
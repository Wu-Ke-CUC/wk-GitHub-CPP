#include<iostream>
using namespace std;

//地址传递
void mySwap1(int* num1, int* num2) noexcept
{
	if (num1 == NULL || num2 == NULL)return;
	int temp = *num1;
	* num1 = *num2;
	* num2 = temp;
}
//引用传递
void mySwap2(int& num1, int& num2) noexcept
{
	int temp = num1;
	num1 = num2;
	num2 = temp;
}

int& test01() noexcept
{
	int a = 10;
	return a;
}
int& test02() noexcept
{
	static int a = 10;
	return a;
}
int main()
{
	//int num1 = 10;
	//int num2 = 20;
	//cout << "num1:" << num1 << ' ' << "num2:" << num2 << endl;
	//mySwap1(&num1, &num2);
	//mySwap2(num1, num2);
	//cout <<"num1:" << num1 << ' ' <<"num2:" << num2 << endl;

	//int& ret1 = test01();
	//cout << "ret:" << ret << endl;

	//const int& ret2 = test02();
	//cout <<"ret2:" << ret2 << endl;
	//test02() = 100;
	//cout << "ret2:" << ret2 << endl;

	//const int& ref = 10;//=>int temp=10; int& ref = temp;//只读
}
#include<iostream>
using namespace std;

#define PI 3.14

class Circle
{
public:										//访问权限
	int radius;								//属性
	double getPerimeter() noexcept			//方法
	{
		return 2 * PI * radius;
	}
};

class Student
{
private:
	string name;
	int age;
	int id;
public:
	Student(string name,int age,int id)
	{
		this->name = name;
		this->age = age;
		this->id = id;
	}
	void setName(string name)
	{
		this->name = name;
	}
	void setAge(int age) noexcept
	{
		if (age >= 0 && age <= 99)
		{
			this->age = age;
			return;
		}
	}
	void setId(int id) noexcept
	{
		this->id = id;
	}
	void showInfo()
	{
		cout << name << "'s id is " << id << "," << age <<" year's old"<< endl;
	}
};

class Cube
{
public:
	int length;
	int width;
	int height;
	Cube(int length, int width, int height) noexcept
	{
		this->length = length;
		this->width = width;
		this->height = height;
	}
	int Area() noexcept
	{
		return 2 * (length * width + length * height + width * height);
	}
	int Volume() noexcept
	{
		return length * width * height;
	}
	bool IsEqualToOther(Cube cube) noexcept
	{
		return (length == cube.length && width == cube.width && height == cube.height);
	}
};

bool CubeIsEqual(Cube cube1, Cube cube2) noexcept
{
	return (cube1.length == cube2.length && cube1.width == cube2.width && cube1.height == cube2.height);
}

int main()
{
	Circle circle;							//实例化
	circle.radius = 2;
	cout << circle.getPerimeter() << endl;

	Student student = Student("ZhangSang", 18, 2026);
	student.setId(2024);
	student.showInfo();
	//cout << student.id << endl;

	Cube cube1 = Cube(10, 10, 10);
	Cube cube2 = Cube(10, 10, 10);
	cout << CubeIsEqual(cube1, cube2) << " " << cube1.IsEqualToOther(cube2) << endl;
	return 0;
}
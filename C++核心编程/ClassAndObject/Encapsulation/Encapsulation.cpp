#include "CircleHead.h"
#include "PointHead.h"

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
	Student(const Student& student)	//拷贝构造
	{
		name = student.name;
		age = student.age;
		id = student.id;
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

class Person
{
public:
	static string name;			 //类内声明
	int age;
	int* height;
	Person(int age,int height):age(age),height(new int(height)){}//初始化列表
	//Person(int age, int height)
	//{
	//	this->age = age;
	//	this->height = new int(height);
	//}
	~Person()
	{
		if (height != NULL)
		{
			delete height;
			height = NULL;
		}
	}
	Person(const Person& person)
	{
		age = person.age;
		height = new int(*person.height);	//深拷贝，重新开辟空间
	}
};
string Person::name = "Zhangsan";//类外初始化

int main()
{
	Circle circle;							//实例化
	Point point1;
	Point point2;
	point1.x = 0, point1.y = 0;
	circle.cPoint = point1;
	circle.radius = 5;
	point2.x = 3, point2.y = 3;
	cout << circle.getPerimeter() << " " << circle.pointIsInCircle(point1) << endl;

	Student student = Student("ZhangSang", 18, 2026);
	student.setId(2024);
	student.showInfo();
	//cout << student.id << endl;

	Cube cube1 = Cube(10, 10, 10);
	Cube cube2 = Cube(10, 10, 10);
	cout << CubeIsEqual(cube1, cube2) << " " << cube1.IsEqualToOther(cube2) << endl;

	Person person(18, 180);
	cout << person.age << " " << *person.height << endl;
	Person person1(person);
	cout << person1.age << " " << *person1.height << endl;

	return 0;
}
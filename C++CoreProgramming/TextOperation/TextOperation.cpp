#include<iostream>
#include<string>
#include<fstream>

using namespace std;

class Person
{
public:
	char name[64];
	int age;
};

int main()
{
	//写文件
	//ofstream ofs;
	//ofs.open("text.txt", ios::app | ios::out);
	//ofs << "1 2 3 4 5" << endl;
	//ofs.close();

	//读文件
	//ifstream ifs;
	//ifs.open("text.txt", ios::in);
	//if (ifs.is_open())
	//{
	//	//char buf[1024] = { 0 };
	//	//方法1
	//	//while (ifs >> buf)
	//	//{
	//	//	cout << buf << endl;
	//	//}
	//	//方法2
	//	//while (ifs.getline(buf, sizeof(buf)))
	//	//{
	//	//	cout << buf << endl;
	//	//}
	//	//方法3
	//	//string buf1;
	//	//while (getline(ifs,buf1))
	//	//{
	//	//	cout << buf1 << endl;
	//	//}
	//	//方法4
	//	//char c;
	//	//while ((c = ifs.get()) != EOF)
	//	//{
	//	//	cout << c;
	//	//}
	//}
	//else
	//{
	//	cout << "open failed" << endl;
	//	return 0;
	//}
	//ifs.close();

	Person p = { "张三",18 };
	ofstream ofs;
	ofs.open("Person.txt", ios::out | ios::binary);
	ofs.write((const char*)&p, sizeof(Person));
	ofs.close();
	ifstream ifs;
	ifs.open("Person.txt", ios::in | ios::binary);
	if (!ifs.is_open())
	{
		cout << "open failed" << endl;
	}
	Person p1;
	ifs.read((char*)&p1, sizeof(Person));
	cout << p1.name << " " << p1.age << endl;
	ifs.close();
	return 0;
}
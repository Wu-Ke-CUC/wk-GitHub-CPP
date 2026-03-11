#include<iostream>
using namespace std;

class Building
{
	friend void goodFriend(const Building& building);

	Building()
	{
		this->livingRoom = "livingRoom";
		this->bdeRoom = "bedRoom";
	}
public:
	string livingRoom;
private:
	string bdeRoom;
};

void goodFirend(const Building& building)
{
	cout << "正在访问：" << building.livingRoom;
	cout << "正在访问：" << building.bdeRoom;
}

int main()
{
	Building building();
}
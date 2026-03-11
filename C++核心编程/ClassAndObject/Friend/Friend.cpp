#include "GoodFriendHead.h"
#include "BuildingHead.h"

void goodFriendVisit(const Building& building)
{
	cout << building.livingRoom << endl;
	cout << building.bedRoom << endl;
}

int main()
{
	Building building;
	goodFriendVisit(building);

	GoodFriend goodFriend;
	goodFriend.visit();
	return 0;
}
#include "GoodFriendHead.h"
#include "BuildingHead.h"
GoodFriend::GoodFriend()
{
	building = new Building;
}
void GoodFriend::visit()
{
	cout << building->livingRoom << endl;
	cout << building->bedRoom << endl;
}
#pragma once
#include "GoodFriendHead.h"
class Building
{
	friend void goodFriendVisit(const Building& building);	//全局函数作为友元
	//friend class GoodFriend;								//类作为友元
	friend void GoodFriend::visit();						//成员函数作为友元

public:
	string livingRoom;
	Building();
private:
	string bedRoom;
};
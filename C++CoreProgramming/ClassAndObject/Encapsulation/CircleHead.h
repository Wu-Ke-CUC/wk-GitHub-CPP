#pragma once
#include "PointHead.h"

class Circle
{
public:										//访问权限
	Point cPoint;
	int radius;								//属性
	double getPerimeter() noexcept;			//方法
	bool pointIsInCircle(Point point) noexcept;
};
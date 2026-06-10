#include "CircleHead.h"
double Circle::getPerimeter() noexcept
{
	return 2 * PI * radius;
}
bool Circle::pointIsInCircle(Point point) noexcept
{
	return sqrt(pow(cPoint.x - point.x, 2) + pow(cPoint.y - point.y, 2)) <= radius;
}
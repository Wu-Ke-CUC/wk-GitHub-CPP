#include<iostream>
using namespace std;

#define MAXSIZE  10
#pragma region SequenceList
typedef struct
{
	int data[MAXSIZE];
	int length;
}SqList;
void InitSqList(SqList& L)
{
	for (int i = 0; i < MAXSIZE; i++)
	{
		L.data[i] = 0;
	}
	L.length = 0;
}
void DeleSqList(SqList*& L)
{
	free(L);
}
bool SqListInsert(SqList& L, int pos, int element)
{
	if (pos<1 || pos>L.length+1)
	{
		return false;
	}
	if (L.length >= MAXSIZE)
	{
		return false;
	}
	for (int i = L.length;i >= pos;i--)
	{
		L.data[i] = L.data[i - 1];
	}
	L.data[pos - 1] = element;
	L.length++;
	return true;
}
bool SqListDelete(SqList& L, int pos)
{
	if (pos<1 || pos>L.length)
	{
		return false;
	}
	for (int i = pos;i < L.length;i++)
	{
		L.data[i - 1] = L.data[i];
	}
	L.length--;
	return true;
}
int SqListSelect(const SqList L,int value)
{
	for (int i = 0;i < L.length;i++)
	{
		if (L.data[i] == value)
		{
			return i + 1;
		}
	}
	return -1;
}
void SqListUpdate(SqList& L, int pos, int newValue)
{
	if (pos<1 || pos>L.length)
	{
		return;
	}
	L.data[pos - 1] = newValue;
}
void Show(const SqList L)
{
	for (int i = 0;i < L.length;i++)
	{
		cout << L.data[i] << "\t";
	}
	cout << endl;
}
#pragma endregion
#pragma region LinkList
typedef struct LNode
{
	int value;
	struct LNode* next;
}LNode, * LinkList;
bool LinkListInsertByPos(LinkList L, int pos,LNode* node)
{
	if (pos < 1)return false;
	LNode* p = L;
	for (int i = 1;i < pos;i++)
	{
		p = p->next;
		if (p == NULL)
		{
			return false;
		}
	}
	node->next = p->next;
	p->next = node;
	return true;
}
bool LinkListInsertBackByValue(LinkList L, int value,LNode* node)
{
	LNode* p = L->next;
	while (p)
	{
		if (p->value != value)
		{
			p = p->next;
		}
		else
		{
			node->next = p->next;
			p->next = node;
			return true;
		}
	}
	return false;
}
bool LinkListInsertForwardByValue(LinkList L, int value, LNode* node)
{
	LNode* p = L;
	while (p->next)
	{
		if (p->next->value != value)
		{
			p = p->next;
		}
		else
		{
			node->next = p->next;
			p->next = node;
			return true;
		}
	}
	return false;
}
LNode* LinkListSelectByPos(LinkList L, int pos)
{
	LNode* p = L;
	if (pos < 1)return NULL;
	for (int i = 0;i < pos;i++)
	{
		p = p->next;
		if (p == NULL)
		{
			return NULL;
		}
	}
	return p;
}
LNode* LinkListSelectByValue(LinkList L, int value)
{
	LNode* p = L->next;
	while (p)
	{
		if (p->value == value)
		{
			return p;
		}
		p = p->next;
	}
	return NULL;
}
int GetLinkListLength(LinkList L)
{
	int length = 0;
	LNode* p = L;
	while (p->next)
	{
		p = p->next;
		length++;
	}
	return length;
}
void LinkListInsertForward(LinkList L,LNode* node)
{
	LinkListInsertByPos(L, 1, node);
}
void LinkListInsertBack(LinkList L, LNode* node)
{
	LNode* p = L;
	while (p->next)
	{
		p = p->next;
	}
	p->next = node;
}
void LinkListReverse(LinkList L)
{
	if (L->next == NULL)return;
	LNode* p = L->next;
	LNode* q;
	while (p->next)
	{
		q = p->next;
		p->next = q->next;
		q->next = L->next;
		L->next = q;
	}
}
#pragma endregion
#pragma region Stack
typedef struct SqStack 
{
	int data[MAXSIZE];
	int top;
}SqStack;
typedef struct LinkStack
{
	LinkList top;
}LinkStack;
void InitSqStack(SqStack& stack)
{
	stack.top = -1;
}
void InitLinkStack(LinkStack& stack)
{
	stack.top = NULL;
}
bool SqStackPush(SqStack& stack,int value)
{
	if (stack.top >= MAXSIZE - 1)return false;
	stack.data[++stack.top] = value;
	return true;
}
void LinkStackPush(LinkStack& stack, LNode* node)
{
	if (node == NULL)return;
	node->next = stack.top;
	stack.top = node;
}
bool SqStackPop(SqStack& stack,int& value)
{
	if (stack.top == -1)return false;
	value = stack.data[stack.top--];
	return true;
}
bool LinkStackPop(LinkStack& stack,LNode* node)
{
	if (stack.top == NULL || node == NULL)return false;
	LNode* top = stack.top;
	node->value = top->value;
	stack.top = top->next;
	free(top);
	return true;
}
bool SqStackIsEmpty(const SqStack& stack)
{
	return stack.top == -1;
}
bool LinkStackIsEmpty(const LinkStack& stack)
{
	return stack.top == NULL;
}
#pragma endregion


int main()
{
	{
		SqList L;
		InitSqList(L);
		SqListInsert(L, 1, 1);
		SqListInsert(L, 2, 2);
		SqListInsert(L, 3, 3);
		Show(L);
		SqListInsert(L, 4, 5);
		Show(L);
		SqListDelete(L, 2);
		Show(L);
		int pos = SqListSelect(L, 3);
		cout << pos << endl;
		SqListUpdate(L, 2, 10);
		Show(L);
		pos = SqListSelect(L, 3);
		cout << pos << endl;
	}
	{

	}
	return 0;
}
#include<iostream>
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<algorithm>
#include<stack>
#include<queue>
#include<string>
using namespace std;
struct ListNode {
	int val;
	ListNode* next;
	ListNode() : val(0), next(nullptr) {}
	ListNode(int x) : val(x), next(nullptr) {}
	ListNode(int x, ListNode* next) : val(x), next(next) {}
};
struct TreeNode {
	int val;
	TreeNode* left;
	TreeNode* right;
	TreeNode() : val(0), left(nullptr), right(nullptr) {}
	TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
	TreeNode(int x, TreeNode* left, TreeNode* right) : val(x), left(left), right(right) {}
};
class Solution 
{
	public:
		#pragma region 1.两数之和
		vector<int> twoSum(vector<int>& nums, int target)
		{
			/*for (int i = 0; i < nums.size(); i++)
			{
				for (int j = i + 1; j < nums.size(); j++)
				{
					if (nums[i] + nums[j] == target)
					{
						return { i, j };
					}
				}
			}
			return {};*/
			unordered_map<int,int> hashTable;
			for (int i = 0; i < nums.size(); i++)
			{
				auto it = hashTable.find(target - nums[i]);
				if (it != hashTable.end())
				{
					return { it->second,i };
				}
				hashTable[nums[i]] = i;
			}
			return {};
		}
		#pragma endregion
		#pragma region 2.两数相加
		ListNode* addTwoNumbers(ListNode* l1, ListNode* l2)
		{
			ListNode* head = nullptr; ListNode* tail = nullptr;
			int carry = 0;
			while (l1 || l2)
			{
				size_t n1 = l1 ? l1->val : 0, n2 = l2 ? l2->val : 0;
				int sum = n1 + n2 + carry;
				if (head == nullptr)
				{
					head = tail = new ListNode(sum % 10);
				}
				else
				{
					tail->next = new ListNode(sum % 10);
					tail = tail->next;
				}
				carry = sum / 10;
				if (l1) { l1 = l1->next; }
				if (l2) { l2 = l2->next; }
			}
			if (carry != 0)
			{
				tail->next = new ListNode(carry);
				tail = tail->next;
			}
			return head;
		}
		#pragma endregion
		#pragma region 3.无重复字符的最长字串
		int lengthOfLongestSubstring(string s) {
			int ret = 0;
			int left = 0;
			unordered_set<char> hashTable;
			for (int right = 0; right < s.length(); right++)
			{
				while(hashTable.count(s[right]))
				{
					hashTable.erase(s[left]);
					left++;
				}
				hashTable.insert(s[right]);
				ret = max(ret, right - left + 1);
			}
			return ret;
		}
		#pragma endregion
		#pragma region 4.寻找两个正序数组的中位数
		double findMedianSortedArrays(vector<int>& nums1, vector<int>& nums2) 
		{
			int n1 = nums1.size();
			int n2 = nums2.size();
			int n = 0, m = 0;
			vector<int> addArray(n1 + n2);
			int temp = 0;
			while (n != n1 && m != n2)
			{
				if (nums1[n] < nums2[m])
				{
					addArray[temp++] = nums1[n++];
				}
				else
				{
					addArray[temp++] = nums2[m++];
				}
			}
			while (n != n1)
			{
				addArray[temp++] = nums1[n++];
			}
			while (m != n2)
			{
				addArray[temp++] = nums2[m++];
			}
			if (temp % 2 == 1)
			{
				return (double)addArray[temp / 2];
			}
			else
			{
				return (addArray[temp / 2 - 1] + addArray[temp / 2]) / (double)2;
			}
		}
		#pragma endregion
		#pragma region 5.最长回文字串
		string longestPalindrome(string s) 
		{
			/*int len = 0;
			string ret = "";
			for (int i = 0; i < s.length(); i++)
			{
				for (int j = s.length() - 1; j >= i; j--) 
				{
					int left = i, right = j;
					int isPalindrome = true;
					while (left <= right)
					{
						if (s[left] != s[right])
						{
							isPalindrome = false;
							break;
						}
						left++; right--;
					}
					if (isPalindrome)
					{
						if (j - i + 1 > len)
						{
							len = j - i + 1;
							ret = s.substr(i, j - i + 1);
						}
					}
				}
			}
			return ret;*/
			//中心扩展算法
			int start = 0, end = 0;
			for (int i = 0; i < s.length(); i++)
			{
				auto [left1, right1] = expendAroundCenter(s, i, i);
				auto [left2, right2] = expendAroundCenter(s, i, i + 1);
				if (right1 - left1 > end - start)
				{
					start = left1;
					end = right1;
				}
				if (right2 - left2 > end - start)
				{
					start = left2;
					end = right2;
				}
			}
			return s.substr(start, end - start + 1);
		}
		pair<int, int> expendAroundCenter(string s, int left, int right)
		{
			while (left >= 0 && right <= s.length() && s[left] == s[right])
			{
				left--, right++;
			}
			return { left + 1,right - 1 };
		}
		#pragma endregion
		#pragma region 6.Z字变换
		string convert(string s, int numRows) 
		{
			if (numRows == 1 || numRows >= s.length()) 
			{
				return s;
			}
			string ret;
			int t = numRows * 2 - 2;
			for (int i = 0; i < numRows; i++)
			{
				for (int j = 0; j + i < s.length(); j += t)
				{
					ret += s[j + i];
					if (i > 0 && i < numRows - 1 && j + t - i < s.length())
					{
						ret += s[j + t - i];
					}
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 7.整数反转
		//int reverse(int x) 
		//{
		//	int ret = 0;
		//	while (x)
		//	{
		//		if (ret > INT32_MAX / 10 || ret < INT32_MIN / 10)
		//		{
		//			return 0;
		//		}
		//		int digit = x % 10;
		//		x /= 10;
		//		ret = ret * 10 + digit;
		//	}
		//	return ret;
		//}
		#pragma endregion
		#pragma region 8.atoi
		int myAtoi(string s) {
			if (s == "")
			{
				return 0;
			}
			int ret = 0;
			int index = 0;
			int symbol = 1;
			while (s[index] < '0' || s[index] > '9' && index < s.size())
			{
				if (s[index] == ' ')
				{
					return 0;
				}
				index++;
			}
			if (index > 0 && s[index - 1] == '-')
			{
				symbol = -1;
			}
			while (s[index] >= '0' && s[index] <= '9' && index<s.size())
			{
				if (ret < INT32_MAX / 10)
				{
					ret = ret * 10 + (s[index++] - '0');
				}
			}
			return ret * symbol;
		}
		#pragma endregion
		#pragma region 9.回文数
		bool isPalindrome(int x) 
		{
			if (x < 0)return false;
			int ret = 0;
			int temp = x;
			while (temp)
			{
				int digit = temp % 10;
				temp /= 10;
				ret = ret * 10 + digit;
				if (ret > INT32_MAX / 10) return false;
			}
			return ret == x ? true : false;
		}
		#pragma endregion
		#pragma region 10.正则表达式匹配 （非自主）
		bool isMatch(string s, string p) 
		{
			int m = s.size();
			int n = p.size();

			// dp[i][j] 表示 s[0..i-1] 是否能匹配 p[0..j-1]
			vector<vector<bool>> dp(m + 1, vector<bool>(n + 1, false));

			dp[0][0] = true;

			// 处理 p 形如 a* a*b* 能匹配空串的情况
			for (int j = 2; j <= n; j++)
			{
				if (p[j - 1] == '*')
					dp[0][j] = dp[0][j - 2];
			}

			for (int i = 1; i <= m; i++)
			{
				for (int j = 1; j <= n; j++)
				{
					// 普通字符 或 '.'
					if (p[j - 1] == '.' || p[j - 1] == s[i - 1])
					{
						dp[i][j] = dp[i - 1][j - 1];
					}
					// '*'
					else if (p[j - 1] == '*')
					{
						// 匹配 0 次
						dp[i][j] = dp[i][j - 2];

						// 匹配 1 次或多次
						if (p[j - 2] == '.' || p[j - 2] == s[i - 1])
						{
							dp[i][j] = dp[i][j] || dp[i - 1][j];
						}
					}
				}
			}

			return dp[m][n];
		}
		#pragma endregion
		#pragma region 11.装水最多的容器
		int maxArea(vector<int>& height) {
			//方法1
			/*int ret = 0;
			for (int i = 0; i < height.size(); i++)
			{
				for (int j = i; j < height.size(); j++)
				{
					int v = min(height[i], height[j]) * (j - i);
					ret = max(ret, v);
				}
			}
			return ret;*/
			//方法2
			int left = 0, right = height.size() - 1;
			int ret = 0;
			while (left < right)
			{
				int tempS = min(height[left], height[right]) * (right - left);
				ret = max(ret, tempS);
				height[left] < height[right] ? left++ : right--;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 12.整数转罗马数字
		string intToRoman(int num) {
			pair<int, string> valueSymbol[] = { {1000, "M"},{900,  "CM"},{500,  "D"},{400,  "CD"},{100,  "C"},
												{90,   "XC"},{50,   "L"},{40,   "XL"},{10,   "X"},{9,    "IX"},
												{5,    "V"},{4,    "IV"},{1,    "I"},};
			string ret = "";
			for (const auto& [value, symbol] : valueSymbol)
			{
				while (num >= value)
				{
					ret += symbol;
					num -= value;
				}
				if (num == 0)break;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 13.罗马数字转整数
		int romanToInt(string s) {
			unordered_map<char, int> symbolValues = {{'I', 1},{'V', 5},{'X', 10},{'L', 50},
													 {'C', 100},{'D', 500},{'M', 1000},};
			int ret = 0;
			for (int i = 0; i < s.length(); i++)
			{
				int value = symbolValues[s[i]];
				if (i < s.length() - 1 && value < symbolValues[s[i + 1]])
				{
					ret -= value;
				}
				else
				{
					ret += value;
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 14.最长公共前缀
		string longestCommonPrefix(vector<string>& strs) {
			string ret = "";
			int index = 0;
			while (true)
			{
				char letter;
				if (strs[0][index]) letter = strs[0][index];
				else break;
				bool isCommon = true;
				for (int i = 0; i < strs.size(); i++)
				{
					if (strs[i][index] != letter)
					{
						isCommon = false;
					}
				}
				if (isCommon)
				{
					ret += letter;
					index++;
				}
				else break;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 15.三数之和
		vector<vector<int>> threeSum(vector<int>& nums) {
			int n = nums.size();
			sort(nums.begin(), nums.end());
			vector<vector<int>> ret;
			for (int first = 0; first < n - 2; first++)
			{
				if (first > 0 && nums[first] == nums[first - 1])
				{
					continue;
				}
				int third = n - 1;
				for (int second = first + 1; second < n - 1; second++)
				{
					if (second > first + 1 && nums[second] == nums[second - 1])
					{
						continue;
					}
					while (second < third && nums[first] + nums[second] + nums[third]>0)
					{
						third--;
					}
					if (second == third)break;
					if (nums[first] + nums[second] + nums[third] == 0)
					{
						ret.push_back({ nums[first] , nums[second] , nums[third] });
					}
					/*for (int third = n - 1; third > second; third--)
					{
						if (third > second + 1 && nums[third] == nums[third - 1])
						{
							continue;
						}
						if (nums[first] + nums[second] + nums[third] == 0)
						{
							ret.push_back({ nums[first] , nums[second] , nums[third] });
						}
					}*/
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 16.最接近的三数之和
		int threeSumClosest(vector<int>& nums, int target) {
			int n = nums.size();
			sort(nums.begin(), nums.end());
			int ret = 1e7;
			for (int first = 0; first < n - 2; first++)
			{
				if (first > 0 && nums[first] == nums[first - 1])
				{
					continue;
				}
				/*for (int second = first + 1; second < n - 1; second++)
				{
					for (int third = n - 1; third > second; third--)
					{
						if (third > second + 1 && nums[third] == nums[third - 1])
						{
							continue;
						}
						if (abs(target - ret) > abs(target - (nums[first] + nums[second] + nums[third])))
						{
							ret = nums[first] + nums[second] + nums[third];
						}
					}
				}*/
				int left = first + 1, right = n - 1;
				while (left < right)
				{
					int sum = nums[first] + nums[left] + nums[right];
					if (sum == target)return target;
					if (abs(target - ret) > abs(target - sum))
					{
						ret = sum;
					}
					if (sum > target)
					{
						while (left < right && nums[right - 1] == nums[right])right--;
						right--;
					}
					else if (sum < target)
					{
						while (left < right && nums[left + 1] == nums[left])left++;
						left++;
					}
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 17.电话号码的字母组合
		vector<string> letterCombinations(string digits) {
			pair<int, vector<string>> symbolValue[] = {{2,{"a","b","c"}},{3,{"d","e","f"}},{4,{"g","h","i"}},{5,{"j","k","l"}},
												{6,{"m","n","o"}},{7,{"p","q","r","s"}},{8,{"t","u","v"}},{9,{"w","x","y","z"}} };
			vector<int> ints;
			for(char digit : digits )
			{
				ints.push_back(digit - '0');
			}
			vector<string> ret = symbolValue[ints[0] - 2].second;
			for (int i = 1; i < ints.size(); i++)
			{
				ret = letterPair(ret, symbolValue[ints[i]-2].second);
			}
			return ret;
		}
		vector<string> letterPair(vector<string> s1, vector<string> s2)
		{
			vector<string> ret;
			for (int i = 0; i < s1.size(); i++)
			{
				for (int j = 0; j < s2.size(); j++)
				{
					ret.push_back(s1[i] + s2[j]);
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 18.四数之和
		vector<vector<int>> fourSum(vector<int>& nums, int target) {
			int n = nums.size();
			sort(nums.begin(), nums.end());
			vector<vector<int>> ret;
			for (int first = 0; first < n - 3; first++)
			{
				if (first > 0 && nums[first] == nums[first - 1])
				{
					continue;
				}
				int third = n - 1;
				for (int second = first + 1; second < n - 2; second++)
				{
					if (second > first + 1 && nums[second] == nums[second - 1])
					{
						continue;
					}
					int third = second + 1, fourth = n - 1;
					while (third < fourth)
					{
						long long sum = (long long)nums[first] + nums[second] + nums[third] + nums[fourth];
						if (sum == target)
						{
							ret.push_back({ nums[first] , nums[second] , nums[third] , nums[fourth] });
							while (third < fourth && nums[third] == nums[third + 1])third++;
							while (third < fourth && nums[fourth] == nums[fourth - 1])fourth--;
							third++, fourth--;
						}
						if (sum > target)fourth--;
						if (sum < target)third++;
					}
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 19.删除链表的倒数第N个结点
		ListNode* removeNthFromEnd(ListNode* head, int n) {
			/*if (head == NULL)return NULL;
			ListNode* temp = new ListNode(0, head);
			int num = 0;
			ListNode* temp1 = head;
			while (temp1)
			{
				temp1 = temp1->next;
				num++;
			}
			if (n > num)return head;
			ListNode* temp2 = temp;
			for (int i = 0; i < num - n; i++)
			{
				temp2 = temp2->next;
			}
			ListNode* del = temp2->next;
			temp2->next = temp2->next->next;
			delete del;
			ListNode* ret = temp->next;
			delete temp;
			return ret;*/
			//快慢指针
			if (head == NULL)return NULL;
			ListNode* temp = new ListNode(0, head);
			ListNode* fast = temp, * slow = temp;
			for (int i = 0; i < n; i++)
			{
				fast = fast->next;
			}
			while (fast->next)
			{
				fast = fast->next;
				slow = slow->next;
			}
			ListNode* del = slow->next;
			slow->next = slow->next->next;
			delete del;
			ListNode* ret = temp->next;
			delete temp;
			return ret;
		}
		#pragma endregion
		#pragma region 20.有效的括号
		bool isValid(string s) {
			stack<char> stk;
			for (char c : s)
			{
				if (c == '(')stk.push(')');
				else if (c == '[')stk.push(']');
				else if (c == '{')stk.push('}');
				else
				{
					if (stk.empty() || stk.top() != c)
						return false;
					stk.pop();
				}
			}
			return stk.empty();
		}
		#pragma endregion
		#pragma region 21.合并两个有序链表
		ListNode* mergeTwoLists(ListNode* list1, ListNode* list2) {
			ListNode* temp = new ListNode(0);
			ListNode* ret = temp;
			while (list1 && list2)
			{
				if (list1->val <= list2->val)
				{
					temp->next = list1;
					temp = temp->next;
					list1 = list1->next;
				}
				else if (list1->val > list2->val)
				{
					temp->next = list2;
					temp = temp->next;
					list2 = list2->next;
				}
			}
			if (list1)temp->next = list1;
			else if (list2)temp->next = list2;
			return ret->next;
		}
		#pragma endregion
		#pragma region 22.括号生成
		vector<string> generateParenthesis(int n) {
			dfs("", n, n);
			return ret;
		}
		vector<string> ret;
		void dfs(const string& str, int left, int right)
		{
			if (left < 0 || left > right)return;
			if (left == 0 && right == 0)
			{
				ret.push_back(str);
				return;
			}
			dfs(str + '(', left - 1, right);
			dfs(str + ')', left, right - 1);
		}
		#pragma endregion
		#pragma region 23.合并K个升序链表
		ListNode* mergeKLists(vector<ListNode*>& lists) {
			/*vector<int> numArray;
			for (ListNode* list : lists)
			{
				while (list)
				{
					numArray.push_back(list->val);
					list = list->next;
				}
			}
			if (numArray.empty()) return nullptr;
			sort(numArray.begin(), numArray.end());
			ListNode* ret = new ListNode(0);
			ListNode* temp = ret;
			for (int i = 0; i < numArray.size(); i++)
			{
				temp->next = new ListNode(numArray[i]);
				temp = temp->next;
			}
			return ret->next;*/

			//分治法
			if (lists.empty())return NULL;
			while (lists.size() > 1)
			{
				vector<ListNode*> merge;
				for (int i = 0; i < lists.size(); i += 2)
				{
					if (i + 1 < lists.size())
						merge.push_back(mergeTwoLists(lists[i], lists[i + 1]));
					else
						merge.push_back(lists[i]);
				}
				lists = merge;
			}
			return lists[0];
		}
		#pragma endregion
		#pragma region 24.两两交换链表中的节点
		ListNode* swapPairs(ListNode* head) {
			ListNode* dummy = new ListNode(0, head);
			ListNode* cur = dummy;
			while (cur->next && cur->next->next)
			{
				ListNode* temp1 = cur->next;
				ListNode* temp2 = cur->next->next;
				temp1->next = temp2->next;
				temp2->next = temp1;
				cur->next = temp2;
				cur = cur->next->next;
			}
			return dummy->next;
			/*if (!head || !head->next)return head;
			ListNode* newhead = head->next;
			head->next = swapPairs(newhead->next);
			newhead->next = head;
			return newhead;*/
		}
		#pragma endregion
		#pragma region 25.K个一组翻转链表
		ListNode* reverseKGroup(ListNode* head, int k) {
			if (head == NULL)return NULL;
			vector<int> ints;
			while (head)
			{
				ints.push_back(head->val);
				head = head->next;
			}
			for (int i = 0; i + k <= ints.size(); i += k)
			{
				for (int j = 0; j < k / 2; j++)
				{
					swap(ints[i + j], ints[i + k - j - 1]);
				}
			}
			ListNode* dummy = new ListNode(0);
			ListNode* temp = dummy;
			for (int i = 0; i < ints.size(); i++)
			{
				temp->next = new ListNode(ints[i]);
				temp = temp->next;
			}
			return dummy->next;
		}
		#pragma endregion
		#pragma region 26.删除有序数组中的重复项
		int removeDuplicates(vector<int>& nums) {
			if (nums.empty())return 0;
			int slow = 1;
			for (int fast = 1; fast < nums.size(); fast++)
			{
				if (nums[fast] != nums[slow - 1])
				{
					nums[slow++] = nums[fast];
				}
			}
			return slow;
		}
		#pragma endregion
		#pragma region 27.移除元素
		int removeElement(vector<int>& nums, int val) {
			if (nums.empty())return 0;
			int slow = 0;
			for (int fast = 0; fast < nums.size(); fast++)
			{
				if (nums[fast] != val)
				{
					nums[slow++] = nums[fast];
				}
			}
			return slow;
		}
		#pragma endregion
		#pragma region 28.找出字符串中第一个匹配的下标
		int strStr(string haystack, string needle) {
			int length1 = haystack.length();
			int length2 = needle.length();
			for (int i = 0; i < length1 - length2 + 1; i++)
			{
				if (haystack[i] == needle[0])
				{
					bool isMatch = true;
					for (int j = 0; j < length2; j++)
					{
						if (haystack[i + j] != needle[j])
						{
							isMatch = false;
						}
					}
					if (isMatch)return i;
				}
			}
			return -1;
		}
		#pragma endregion
		#pragma region 29.两数相除 (非自主)
		int divide(int dividend, int divisor) {
			// 考虑被除数为最小值的情况
			if (dividend == INT_MIN) {
				if (divisor == 1) {
					return INT_MIN;
				}
				if (divisor == -1) {
					return INT_MAX;
				}
			}
			// 考虑除数为最小值的情况
			if (divisor == INT_MIN) {
				return dividend == INT_MIN ? 1 : 0;
			}
			// 考虑被除数为 0 的情况
			if (dividend == 0) {
				return 0;
			}

			// 一般情况，使用二分查找
			// 将所有的正数取相反数，这样就只需要考虑一种情况
			bool rev = false;
			if (dividend > 0) {
				dividend = -dividend;
				rev = !rev;
			}
			if (divisor > 0) {
				divisor = -divisor;
				rev = !rev;
			}

			// 快速乘
			auto quickAdd = [](int y, int z, int x) {
				// x 和 y 是负数，z 是正数
				// 需要判断 z * y >= x 是否成立
				int result = 0, add = y;
				while (z) {
					if (z & 1) {
						// 需要保证 result + add >= x
						if (result < x - add) {
							return false;
						}
						result += add;
					}
					if (z != 1) {
						// 需要保证 add + add >= x
						if (add < x - add) {
							return false;
						}
						add += add;
					}
					// 不能使用除法
					z >>= 1;
				}
				return true;
				};

			int left = 1, right = INT_MAX, ans = 0;
			while (left <= right) {
				// 注意溢出，并且不能使用除法
				int mid = left + ((right - left) >> 1);
				bool check = quickAdd(divisor, mid, dividend);
				if (check) {
					ans = mid;
					// 注意溢出
					if (mid == INT_MAX) {
						break;
					}
					left = mid + 1;
				}
				else {
					right = mid - 1;
				}
			}

			return rev ? -ans : ans;
		}
		#pragma endregion
		#pragma region 30.串联所以单词的子串 (非自主)
		vector<int> findSubstring(string s, vector<string>& words) {
			vector<int> res;
			int m = words.size(), n = words[0].size(), ls = s.size();
			for (int i = 0; i < n && i + m * n <= ls; ++i) {
				unordered_map<string, int> differ;
				for (int j = 0; j < m; ++j) {
					++differ[s.substr(i + j * n, n)];
				}
				for (string& word : words) {
					if (--differ[word] == 0) {
						differ.erase(word);
					}
				}
				for (int start = i; start < ls - m * n + 1; start += n) {
					if (start != i) {
						string word = s.substr(start + (m - 1) * n, n);
						if (++differ[word] == 0) {
							differ.erase(word);
						}
						word = s.substr(start - n, n);
						if (--differ[word] == 0) {
							differ.erase(word);
						}
					}
					if (differ.empty()) {
						res.emplace_back(start);
					}
				}
			}
			return res;
		}
		#pragma endregion
		#pragma region 31.下一个排列
		void nextPermutation(vector<int>& nums) {
			int small = nums.size() - 2;
			while (small >= 0 && nums[small] >= nums[small + 1])
			{
				small--;
			}
			if (small >= 0)
			{
				int big = nums.size() - 1;
				while (big >= 0 && nums[big] <= nums[small])
				{
					big--;
				}
				swap(nums[small], nums[big]);
			}
			int left = small + 1;
			int right = nums.size() - 1;
			while (left < right)
			{
				swap(nums[left++], nums[right--]);
			}
		}
		#pragma endregion
		#pragma region 32.最长有效括号 (非自主)
		int longestValidParentheses(string s) 
		{
			int maxRet = 0;
			stack<int> stk;
			stk.push(-1);
			for (int i = 0; i < s.length(); i++) 
			{
				if (s[i] == '(') 
				{
					stk.push(i);
				}
				else 
				{
					stk.pop();
					if (stk.empty()) 
					{
						stk.push(i);
					}
					else 
					{
						maxRet = max(maxRet, i - stk.top());
					}
				}
			}
			return maxRet;
		}
		#pragma endregion
		#pragma region 33.搜索旋转排序数组
		int search(vector<int>& nums, int target) 
		{
			if (nums.empty())return -1;
			if (nums.size() == 1)
			{
				return nums[0] == target ? 0 : -1;
			}
			int left = 0, right = nums.size() - 1;
			while (left <= right)
			{
				int mid = (left + right) / 2;
				if (nums[mid] == target)return mid;
				if (nums[left] <= nums[mid])
				{
					if (target >= nums[left] && target < nums[mid])
					{
						right = mid - 1;
					}
					else
					{
						left = mid + 1;
					}
				}
				else
				{
					if (target <= nums[right] && target > nums[mid])
					{
						left = mid + 1;
					}
					else
					{
						right = mid - 1;
					}
				}
			}
			return -1;
		}
		#pragma endregion
		#pragma region 34.在数组排序中查找元素的第一个和最后一个位置
		vector<int> searchRange(vector<int>& nums, int target) {
			if (nums.empty())return { -1,-1 };
			int left = 0, right = nums.size() - 1;
			while (left <= right)
			{
				int mid = (left + right) / 2;
				if (nums[mid] == target)
				{
					int first = mid, last = mid;
					while (first > 0 && nums[first - 1] == target)first--;
					while (last < nums.size() - 1 && nums[last + 1] == target)last++;
					return { first,last };
				}
				else if (nums[mid] > target)
				{
					right = mid - 1;
				}
				else if (nums[mid] < target)
				{
					left = mid + 1;
				}
			}
			return { -1,-1 };
		}
		#pragma endregion
		#pragma region 35.搜索插入位置
		int searchInsert(vector<int>& nums, int target) {
			int left = 0, right = nums.size() - 1;
			while (left <= right)
			{
				int mid = (left + right) / 2;
				if (nums[mid] == target)
				{
					return mid;
				}
				else if (nums[mid] > target)
				{
					right = mid - 1;
				}
				else if (nums[mid] < target)
				{
					left = mid + 1;
				}
			}
			return left;
		}
		#pragma endregion
		#pragma region 36.有效的数独
		bool isValidSudoku(vector<vector<char>>& board) {
			//竖
			for (int i = 0; i < 9; i++)
			{
				unordered_set<char> hashSet;
				for (int j = 0; j < 9; j++)
				{
					if (board[i][j] > '0' && board[i][j] <= '9')
					{
						if (hashSet.count(board[i][j]))
						{
							return false;
						}
						else
						{
							hashSet.insert(board[i][j]);
						}
					}
				}
			}
			//横
			for (int i = 0; i < 9; i++)
			{
				unordered_set<char> hashSet;
				for (int j = 0; j < 9; j++)
				{
					if (board[j][i] > '0' && board[j][i] <= '9')
					{
						if (hashSet.count(board[j][i]))
						{
							return false;
						}
						else
						{
							hashSet.insert(board[j][i]);
						}
					}
				}
			}
			//九宫格
			for (int a = 0; a < 9; a += 3)
			{
				for (int b = 0; b < 9; b += 3)
				{
					unordered_set<char> hashSet;
					for (int i = 0; i < 3; i++)
					{
						for (int j = 0; j < 3; j++)
						{
							if (board[a + i][b + j] > '0' && board[a + i][b + j] <= '9')
							{
								if (hashSet.count(board[a + i][b + j]))
								{
									return false;
								}
								else
								{
									hashSet.insert(board[a + i][b + j]);
								}
							}
						}
					}
				}
			}
			return true;
		}
		#pragma endregion
		//#pragma region 37.解数独 //非自主
		//int line[9];
		//int column[9];
		//int block[3][3];
		//bool valid;
		//vector<pair<int, int>> spaces;

		//void flip(int i, int j, int digit) {
		//	line[i] ^= (1 << digit);
		//	column[j] ^= (1 << digit);
		//	block[i / 3][j / 3] ^= (1 << digit);
		//}

		//void dfs(vector<vector<char>>& board, int pos) {
		//	if (pos == spaces.size()) {
		//		valid = true;
		//		return;
		//	}

		//	auto [i, j] = spaces[pos];
		//	int mask = ~(line[i] | column[j] | block[i / 3][j / 3]) & 0x1ff;
		//	for (; mask && !valid; mask &= (mask - 1)) {
		//		int digitMask = mask & (-mask);
		//		int digit = __builtin_ctz(digitMask);
		//		flip(i, j, digit);
		//		board[i][j] = digit + '0' + 1;
		//		dfs(board, pos + 1);
		//		flip(i, j, digit);
		//	}
		//}

		//void solveSudoku(vector<vector<char>>& board) {
		//	memset(line, 0, sizeof(line));
		//	memset(column, 0, sizeof(column));
		//	memset(block, 0, sizeof(block));
		//	valid = false;

		//	for (int i = 0; i < 9; ++i) {
		//		for (int j = 0; j < 9; ++j) {
		//			if (board[i][j] == '.') {
		//				spaces.emplace_back(i, j);
		//			}
		//			else {
		//				int digit = board[i][j] - '0' - 1;
		//				flip(i, j, digit);
		//			}
		//		}
		//	}

		//	dfs(board, 0);
		//}
		//#pragma endregion
		#pragma region 38.外观数列
		string countAndSay(int n) {
			if (n == 1)return "1";
			string behind = countAndSay(n - 1);
			string ret = "";
			for (int i = 0; i < behind.length(); i++)
			{
				char c = behind[i];
				int num = 1;
				while (c == behind[i + 1])
				{
					i++;
					num++;
				}
				ret += to_string(num) + c;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 39.组合总数
		void dfs1(vector<int>& candidates, int target, vector<vector<int>>& ret, vector<int>& ele,int dep)
		{
			if (dep == candidates.size())return;
			if (target == 0)
			{
				ret.push_back(ele);
				return;
			}
			dfs1(candidates, target, ret, ele, dep + 1);
			if (target - candidates[dep] >= 0)
			{
				ele.push_back(candidates[dep]);
				dfs1(candidates, target - candidates[dep], ret, ele, dep);
				ele.pop_back();
			}
		}
		vector<vector<int>> combinationSum(vector<int>& candidates, int target) {
			vector<vector<int>>ret;
			vector<int>ele;
			dfs1(candidates, target, ret, ele, 0);
			return ret;
		}
		#pragma endregion
		#pragma region 40.组合总数2
		void dfs2(vector<int>& candidates, int target, vector<vector<int>>& ret, vector<int>& ele, int dep)
		{
			if (target == 0)
			{
				ret.push_back(ele);
				return;
			}
			for (int i = dep; i < candidates.size(); i++)
			{
				if (i > dep && candidates[i] == candidates[i - 1])
					continue;
				if (candidates[i] > target)
					break;
				ele.push_back(candidates[i]);
				dfs2(candidates, target - candidates[i], ret, ele, i + 1);
				ele.pop_back();
			}
		}
		vector<vector<int>> combinationSum2(vector<int>& candidates, int target) {
			sort(candidates.begin(), candidates.end());
			vector<vector<int>>ret;
			vector<int>ele;
			dfs2(candidates, target, ret, ele, 0);
			return ret;
		}
		#pragma endregion
		#pragma region 41.缺失的第一个正数
		int firstMissingPositive(vector<int>& nums) {
			sort(nums.begin(), nums.end());
			int ret = 1;
			for (int i = 0; i < nums.size(); i++)
			{
				if (nums[i] <= 0)continue;
				else if (i > 0 && nums[i] == nums[i - 1]) 
				{
					continue;
				}
				else if (ret == nums[i])
				{
					ret++;
					continue;
				}
				return ret;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 42.接雨水
		int trap(vector<int>& height) {
			int ret = 0;
			int left = 0, right = height.size() - 1;
			int leftMax = 0, rightMax = 0;
			while (left < right)
			{
				leftMax = max(leftMax, height[left]);
				rightMax = max(rightMax, height[right]);
				if (leftMax < rightMax)
				{
					ret += leftMax - height[left];
					left++;
				}
				else
				{
					ret += rightMax - height[right];
					right--;
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 43.字符串相乘
		void stringToIntArray(string s,int* array)
		{
			for (int i = 0; i < s.length(); i++)
			{
				array[s.length() - 1 - i] = s[i] - '0';
			}
		}
		string multiply(string num1, string num2) {
			if (num1 == "0" || num2 == "0")return "0";
			int* a = new int[num1.length()];
			int* b = new int[num2.length()];
			int maxLen = num1.size() + num2.size();
			int* nums = new int[maxLen] {0};
			stringToIntArray(num1, a);
			stringToIntArray(num2, b);
			for (int i = 0; i < num1.length(); i++)
			{
				for (int j = 0; j < num2.length(); j++)
				{
					nums[i + j] += a[i] * b[j];
					nums[i + j + 1] += nums[i + j] / 10;
					nums[i + j] %= 10;
				}
			}
			while (nums[maxLen - 1] == 0 && maxLen > 1)
			{
				maxLen--;
			}
			string ret = "";
			for (int i = 1; i <= maxLen; i++)
			{
				ret += (nums[maxLen - i] + '0');
			}
			return ret;
		}
		#pragma endregion
		#pragma region 44.题目同10
		#pragma endregion
		#pragma region 45.跳跃游戏2
		int jump(vector<int>& nums) {
			int len = nums.size();
			if (len == 1)return 0;
			int index = 0;
			int ret = 0;
			while (index < len)
			{
				if (index + nums[index] >= len - 1)return ret + 1;
				int maxIndex = 0;
				int preIndex = 0;
				int tempIndex = index;
				for (int i = 1; i <= nums[tempIndex]; i++)
				{
					maxIndex = max(maxIndex, tempIndex + i + nums[tempIndex + i]);
					if (maxIndex != preIndex)
					{
						preIndex = maxIndex;
						index = tempIndex + i;
					}
				}
				ret++;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 46.全排列
		void dfs3(vector<vector<int>>& ret, vector<int>& output, int start, int end)
		{
			if (start == end)
			{
				ret.emplace_back(output);
				return;
			}
			for (int i = start; i < end; i++)
			{
				swap(output[i], output[start]);
				dfs3(ret, output, start + 1, end);
				swap(output[i], output[start]);
			}
		}
		vector<vector<int>> permute(vector<int>& nums) {
			vector<vector<int>> ret;
			dfs3(ret, nums, 0, nums.size());
			return ret;
		}
		#pragma endregion
		#pragma region 47.全排列2
		void dfs4(vector<vector<int>>& ret, vector<int>& output, int start, int end)
		{
			if (start == end)
			{
				ret.emplace_back(output);
				return;
			}
			unordered_set<int> usedNum;
			for(int i = start; i < end; i++)
			{
				if (usedNum.find(output[i]) != usedNum.end())continue;
				usedNum.insert(output[i]);
				swap(output[i], output[start]);
				dfs4(ret, output, start + 1, end);
				swap(output[i], output[start]);
		    }
		}
		vector<vector<int>> permuteUnique(vector<int>& nums) 
		{
			vector<vector<int>> ret;
			dfs4(ret, nums, 0, nums.size());
			return ret;
		}
		#pragma endregion
		#pragma region 48.旋转图像
		void rotate(vector<vector<int>>& matrix) 
		{
			int len = matrix.size();
			vector<vector<int>> ret = matrix;
			for (int i = 0; i < len; i++)
			{
				for (int j = 0; j < len; j++)
				{
					ret[j][len - 1 - i] = matrix[i][j];
				}
			}
			matrix = ret;
		}
		#pragma endregion
		#pragma region 49.字母异位词分组
		vector<vector<string>> groupAnagrams(vector<string>& strs) {
			unordered_map<string, vector<string>>mp;
			for (int i = 0; i < strs.size(); i++)
			{
				string key = strs[i];
				sort(key.begin(), key.end());
				mp[key].push_back(strs[i]);
			}
			vector<vector<string>> ret;
			for (auto it = mp.begin(); it != mp.end(); it++)
			{
				ret.push_back(it->second);
			}
			return ret;
		}
		#pragma endregion
		#pragma region 50.Pow(x,n)
		double myPow(double x, int n) {
			if (n == 0 && n < 0)return 0;
			long long exp = n;
			if (n < 0)
			{
				x = 1 / x; exp = -exp;
			}
			double ret = 1;
			while (exp > 0)
			{
				if (exp & 1)ret *= x;
				x *= x;
				exp >>= 1;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 53.最大子数组和
		int maxSubArray(vector<int>& nums) {
			int temp = 0; int maxSum = nums[0];
			for (int x : nums)
			{
				temp = max(temp + x, x);
				maxSum = max(maxSum, temp);
			}
			return maxSum;
		}
		#pragma endregion
		#pragma region 54.螺旋数组
		vector<int> spiralOrder(vector<vector<int>>& matrix) {
			int left = 0, right = matrix[0].size() - 1;
			int head = 0, bottom = matrix.size() - 1;
			vector<int> ret;
			while (left <= right && head <= bottom)
			{
				for (int r1 = left; r1 <= right; r1++)
				{
					ret.push_back(matrix[head][r1]);
				}
				head++;
				for (int c1 = head; c1 <= bottom; c1++)
				{
					ret.push_back(matrix[c1][right]);
				}
				right--;
				if (head <= bottom)
				{
					for (int r2 = right; r2 >= left; r2--)
					{
						ret.push_back(matrix[bottom][r2]);
					}
					bottom--;
				}
				if (left <= right)
				{
					for (int c2 = bottom; c2 >= head; c2--)
					{
						ret.push_back(matrix[c2][left]);
					}
					left++;
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 55.跳跃游戏
		//bool dfs5(vector<int>& nums, int index)
		//{
		//	if (nums[index] == 0)return false;
		//	if (index >= nums.size() - 1)return true;
		//	vector<bool> st;
		//	for (int i = 1; i <= nums[index]; i++)
		//	{
		//		if (dfs5(nums, index + i))
		//		{
		//			return true;
		//		}
		//	}
		//	return false;
		//}
		//bool canJump(vector<int>& nums) {
		//	return dfs5(nums, 0);
		//}
		bool canJump(vector<int>& nums) {
			int maxReach = 0;
			for (int i = 0; i < nums.size(); i++)
			{
				if (i > maxReach)return false;
				maxReach = max(maxReach, i + nums[i]);
				if (maxReach + nums[i] >= nums.size() - 1)return true;
			}
			return true;
		}
		#pragma endregion
		#pragma region 56.合并区间
		vector<vector<int>> merge(vector<vector<int>>& intervals) {
			if (intervals.size() == 0)return {};
			sort(intervals.begin(), intervals.end());
			vector<vector<int>> ret;
			for (int i = 0; i < intervals.size(); ++i)
			{
				int L = intervals[i][0], R = intervals[i][1];
				if (ret.size() == 0 || ret.back()[1] < L)
				{
					ret.push_back({L,R});
				}
				else if (ret.back()[1] >= L && ret.back()[1] < R)
				{
					ret.back()[1] = R;
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 57.插入区间
		vector<vector<int>> insert(vector<vector<int>>& intervals, vector<int>& newInterval) {
			int left = newInterval[0], right = newInterval[1];
			vector<vector<int>> ret;
			bool isPlaced = false;
			for (int i = 0; i < intervals.size(); i++)
			{
				if (intervals[i][0] > right)
				{
					if (!isPlaced)
					{
						ret.push_back({ left,right });
						isPlaced = true;
					}
					ret.push_back(intervals[i]);
				}
				else if (intervals[i][1] < left)
				{
					ret.push_back(intervals[i]);
				}
				else
				{
					left = min(left, intervals[i][0]);
					right = max(right, intervals[i][1]);
				}
			}
			if (!isPlaced)ret.push_back({ left,right });
			return ret;
		}
		#pragma endregion
		#pragma region 58.最后一个单词长度
		int lengthOfLastWord(string s) {
			if (s.size() == 0)return 0;
			int index = s.size() - 1;
			while (index >=0 &&s[index] == ' ')
			{
				index--;
			}
			int ret = 0;
			while (index >= 0 && s[index] != ' ')
			{
				ret++;
				index--;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 59.螺旋矩阵2
		vector<vector<int>> generateMatrix(int n) {
			vector<vector<int>> ret(n, vector<int>(n));
			int top = 0, bottom = n - 1;
			int left = 0, right = n - 1;
			int num = 1;
			while (top <= bottom && left <= right)
			{
				for (int i = left; i <= right; i++)
				{
					ret[top][i] = num++;
				}
				top++;
				for (int i = top; i <= bottom; i++)
				{
					ret[i][right] = num++;
				}
				right--;
				if (left <= right)
				{
					for (int i = right; i >= left; i--)
					{
						ret[bottom][i] = num++;
					}
					bottom--;
				}
				if (top <= bottom)
				{
					for (int i = bottom; i >= top; i--)
					{
						ret[i][left] = num++;
					}
					left++;
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 60.排列序列
		string getPermutation(int n, int k) {
			vector<int> factorial(n);
			factorial[0] = 1;
			for (int i = 1; i < n; i++)
			{
				factorial[i] = factorial[i - 1] * i;
			}
			--k;
			vector<int> valid(n + 1, 1);
			string ret = "";
			for (int i = 1; i <= n; i++)
			{
				int order = k / factorial[n - i] + 1;
				for (int j = 1; j <= n; j++)
				{
					order -= valid[j];
					if (order == 0)
					{
						ret += j + '0';
						valid[j] = 0;
						break;
					}
				}
				k %= factorial[n - i];
			}
			return ret;
		}
		#pragma endregion
		#pragma region 61.旋转链表
		ListNode* rotateRight(ListNode* head, int k) {
			if (head == NULL || k == 0)return head;
			ListNode* slow = head;
			ListNode* fast = head;
			int n = 1;
			for (int i = 0; i < k; i++)
			{
				if (!fast->next)
				{
					fast = head;
					i = k / n * n - 1;
					continue;
				}
				fast = fast->next;
				n++;
			}
			while (fast->next)
			{
				slow = slow->next;
				fast = fast->next;
			}
			fast->next = head;
			ListNode* ret = slow->next;
			slow->next = NULL;
			return ret;
		}
		#pragma endregion
		#pragma region 62.不同路径
		//方法一
		//int dfs5(int x, int y, int m, int n, vector<vector<int>> memo)
		//{
		//	if (x == n || y == m)return 1;
		//	if (memo[y][n] != -1) return memo[y][x];
		//	memo[y][x] = dfs5(x + 1, y, m, n, memo) + dfs5(x, y + 1, m, n, memo);
		//	return memo[y][x];
		//}
		//int uniquePaths(int m, int n) {
		//	vector<vector<int>> memo(m + 1, vector<int>(n + 1, -1));
		//	return dfs5(1, 1, m, n, memo);
		//}
		//方法二
		int uniquePaths(int m, int n) {
			vector<vector<int>> dp(m, vector<int>(n, 1));
			for (int i = 1; i < m; i++)
			{
				for (int j = 1; j < n; j++)
				{
					dp[i][j] = dp[i - 1][j] + dp[i][j - 1];
				}
			}
			return dp[m - 1][n - 1];
		}
		//方法三
		//int Factorial(int num)
		//{
		//	if (num == 0)return 1;
		//	return num * Factorial(num - 1);
		//}
		//int Combnation(int m, int n)
		//{
		//	if (n > m)return Combnation(n, m);
		//	return Factorial(m) / (Factorial(n) * Factorial(m - n));
		//}
		//int uniquePaths(int m, int n) {
		//	return Combnation(m + n - 2, n - 1);
		//}
		#pragma endregion
		#pragma region 63.不同路径2
		int uniquePathsWithObstacles(vector<vector<int>>& obstacleGrid) {
			int row = obstacleGrid.size();
			int col = obstacleGrid[0].size();
			vector<vector<int>> dp(row, vector<int>(col, 0));
			dp[0][0] = (obstacleGrid[0][0] == 0);
			for (int i = 0; i < row; i++)
			{
				for (int j = 0; j < col; j++)
				{
					if (obstacleGrid[i][j] == 1)
					{
						dp[i][j] = 0;
						continue;
					}
					if (i >= 1 && j == 0)
					{
						dp[i][j] = dp[i - 1][j];
					}
					else if (i == 0 && j >= 1)
					{
						dp[i][j] = dp[i][j - 1];
					}
					else if(i >= 1 && j >= 1)
					{
						dp[i][j] = dp[i - 1][j] + dp[i][j - 1];
					}
				}
			}
			return dp[row - 1][col - 1];
		}
		#pragma endregion
		#pragma region 64.最小路径和
		//方法1
		//int dfs6(vector<vector<int>>& grid, int x, int y, int m, int n,vector<vector<int>>& memo)
		//{
		//	if (x == m - 1 && y == n - 1)return grid[x][y];
		//	if (memo[x][y] != -1)return memo[x][y];
		//	else if (x == m - 1)
		//	{
		//		return grid[x][y] + dfs6(grid, x, y + 1, m, n, memo);
		//	}
		//	else if (y == n - 1)
		//	{
		//		return grid[x][y] + dfs6(grid, x + 1, y, m, n, memo);
		//	}
		//	else
		//	{
		//		return grid[x][y] + min(dfs6(grid, x, y + 1, m, n, memo), dfs6(grid, x + 1, y, m, n, memo));
		//	}
		//}
		//int minPathSum(vector<vector<int>>& grid) {
		//	int m = grid.size();
		//	int n = grid[0].size();
		//	vector<vector<int>>memo(m, vector<int>(n, -1));
		//	return dfs6(grid, 0, 0, m, n, memo);
		//}
		//方法2
		//int minPathSum(vector<vector<int>>& grid) {
		//	int row = grid.size();
		//	int col = grid[0].size();
		//	if (row == 0 || col == 0)return 0;
		//	vector<vector<int>> dp(row, vector<int>(col));
		//	dp[0][0] = grid[0][0];
		//	for (int i = 1; i < row; i++)
		//	{
		//		dp[i][0] = dp[i - 1][0] + grid[i][0];
		//	}
		//	for (int i = 1; i < col; i++)
		//	{
		//		dp[0][i] = dp[0][i - 1] + grid[0][i];
		//	}
		//	for (int i = 1; i < row; i++)
		//	{
		//		for (int j = 1; j < col; j++)
		//		{
		//			dp[i][j] = min(dp[i - 1][j], dp[i][j - 1]) + grid[i][j];
		//		}
		//	}
		//	return dp[row - 1][col - 1];
		//}
		//方法3
		int minPathSum(vector<vector<int>>& grid){
			int row = grid.size(), col = grid[0].size();
			if (row == 0 || col == 0)return 0;
			vector<int> dp(col);
			for (int i = 0; i < row; i++)
			{
				dp[0] += grid[i][0];
				for (int j = 1; j < col; j++)
				{
					dp[j] = (i > 0 ? min(dp[j - 1], dp[j]) : dp[j - 1]) + grid[i][j];
				}
			}
			return dp[col - 1];
		}
		#pragma endregion
		#pragma region 65.有效数字
		bool isNumber(string s) {

		}
		#pragma endregion
		#pragma region 66.加一
		vector<int> plusOne(vector<int>& digits) {
			int n = digits.size();
			for (int i = n - 1; i >= 1; i--)
			{
				if (digits[i] == 9)
				{
					digits[i] = 0;
					continue;
				}
				digits[i] += 1;
				return digits;
			}
			vector<int> ret(n + 1);
			ret[0] = 1;
			return ret;
		}
		#pragma endregion
		#pragma region 67.二进制求和
		string addBinary(string a, string b) {
			string ret;
			reverse(a.begin(), a.end());
			reverse(b.begin(), b.end());
			int n = max(a.size(), b.size()), carry = 0;
			for (int i = 0; i < n; i++)
			{
				carry += i < a.size() ? (a[i] == '1') : 0;
				carry += i < b.size() ? (b[i] == '1') : 0;
				ret.push_back(carry % 2 ? '1' : '0');
				carry /= 2;
			}
			if (carry)
			{
				ret.push_back('1');
			}
			reverse(ret.begin(), ret.end());
			return ret;
		}
		#pragma endregion
		#pragma region 68.文本左右对齐
		vector<string> fullJustify(vector<string>& words, int maxWidth) {
			vector<string> ret;
			for (int start = 0; start < words.size();)
			{
				string temp;
				int keyNum = 0;
				int end = start;
				while(end < words.size())
				{
					if (keyNum + words[end].size() + (end - start) <= maxWidth)
					{
						keyNum += words[end].size();
						end++;
					}
					else break;
				}
				int spaceNum;
				if (end - start == 1)spaceNum = maxWidth - keyNum;
				else spaceNum = (maxWidth - keyNum) / (end - start - 1);
				for (int i = 0; i < words[start].size(); i++)
				{
					temp.push_back(words[start][i]);
				}
				int spaceAdd = maxWidth - keyNum - spaceNum * (end - start - 1);
				if (end == words.size())
				{
					spaceNum = 1;
					spaceAdd = 0;
				}
				for (int i = start + 1; i < end; i++)
				{
					for (int j = 0; j < spaceNum; j++)
					{
						temp.push_back(' ');
					}
					if (spaceAdd > 0)
					{
						temp.push_back(' ');
						spaceAdd--;
					}
					for (int k = 0; k < words[i].size(); k++)
					{
						temp.push_back(words[i][k]);
					}
				}
				if (end - start == 1 || end == words.size())
				{
					for (int i = 0; i < maxWidth - keyNum - (end - start - 1); i++)
					{
						temp.push_back('!');
					}
				}
				start = end;
				ret.push_back(temp);
			}
			return ret;
		}
		#pragma endregion
		#pragma region 69.x的平方根
		//int mySqrt(int x) {
		//	int ret = 1;
		//	while (pow(ret + 1, 2) < x)
		//	{
		//		ret++;
		//	}
		//	return ret;
		//}
		int mySqrt(int x) {
			int left = 0;
			int right = x;
			int ret = 0;
			while (left <= right)
			{
				int mid = right + (left - right) / 2;
				if ((long long)mid * mid <= x)
				{
					ret = mid;
					right = mid + 1;
				}
				else
				{
					left = mid - 1;
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 70.爬楼梯
		int climbStairs(int n) {
			if (n == 1)return 1;
			if (n == 2)return 2;
			int bef2 = 1, bef1 = 2;
			int ret;
			for (int i = 3; i <= n; i++)
			{
				ret = bef2 + bef1;
				bef2 = bef1;
				bef1 = ret;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 73.矩阵置零
		//void setZeroes(vector<vector<int>>& matrix) {
		//	vector<vector<int>> temp = matrix;
		//	for (int i = 0; i < matrix.size(); i++)
		//	{
		//		for (int j = 0; j < matrix[0].size(); j++)
		//		{
		//			if (temp[i][j] == 0)
		//			{
		//				for (int r = 0; r < matrix.size(); r++)
		//				{
		//					matrix[r][j] = 0;
		//				}
		//				for (int c = 0; c < matrix[0].size(); c++)
		//				{
		//					matrix[i][c] = 0;
		//				}
		//			}
		//		}
		//	}
		//}
		void setZeroes(vector<vector<int>>& matrix) {
			int m = matrix.size();
			int n = matrix[0].size();
			vector<int>row(m), col(n);
			for (int i = 0; i < m; i++)
			{
				for (int j = 0; j < n; j++)
				{
					if (matrix[i][j] == 0)
					{
						row[i] = col[j] = true;
					}
				}
			}
			for (int i = 0; i < m; i++)
			{
				for (int j = 0; j < n; j++)
				{
					if (row[i] || col[j])
					{
						matrix[i][j] = 0;
					}
				}
			}
		}
		#pragma endregion
		#pragma region 74.搜索二维矩阵
		bool searchMatrix(vector<vector<int>>& matrix, int target) {
			int m = matrix.size();
			int n = matrix[0].size();
			//for (int i = 0; i < m; i++)
			//{
			//	if (target <= matrix[i][n - 1])
			//	{
			//		int left = 0, right = n - 1;
			//		while (left <= right)
			//		{
			//			int mid = (left + right) / 2;
			//			if (matrix[i][mid] == target)return true;
			//			if (matrix[i][mid] < target)left = mid + 1;
			//			else if (matrix[i][mid] > target)right = mid - 1;
			//		}
			//		return false;
			//	}
			//}
			//return false;
			int top = 0, bottom = m - 1;
			int row = -1;
			while (top <= bottom)
			{
				int mid = (top + bottom) / 2;
				if (matrix[mid][n - 1] == target)return true;
				if (matrix[mid][n - 1] > target)
				{
					row = mid;
					bottom = mid - 1;
				}
				else top = mid + 1;
			}
			if (row == -1)return false;
			int left = 0, right = n - 1;
			while (left <= right)
			{
				int mid = (left + right) / 2;
				if (matrix[row][mid] == target)return true;
				if (matrix[row][mid] > target)right = mid - 1;
				else left = mid + 1;
			}
			return false;
		}
		#pragma endregion
		#pragma region 75.颜色分类
		void sortColors(vector<int>& nums) {
			const int right = nums.size() - 1;
			quickSort(nums, 0, right);
		}
		void quickSort(vector<int>& num, int left, int right)
		{
			if (left > right)return;
			const int base = num[left];
			int l = left, r = right;
			while (l != r)
			{
				while (l < r && num[r] >= base)r--;
				if (l < r)num[l++] = num[r];
				while (l < r && num[l] <= base)l++;
				if (l < r)num[r--] = num[l];
			}
			num[l] = base;
			quickSort(num, left, l - 1);
			quickSort(num, l + 1, right);
		}
		#pragma endregion
		#pragma region 76.最小覆盖子串
		string minWindow(string s, string t) {
			unordered_map<char, int> need, window;  // need记录t中字符需求，window记录窗口内字符出现次数
			for (char c : t) need[c]++;              // 统计t中每个字符的个数
			int left = 0, right = 0;                 // 滑动窗口左右指针
			int valid = 0;                            // 记录窗口中已满足需求（字符出现次数达标）的字符种类数
			int start = 0, len = INT_MAX;             // 记录最小子串的起始位置和长度
			while (right < s.size()) {
				char c = s[right];                     // 即将移入窗口的字符
				right++;                                // 扩大窗口
				if (need.count(c)) {                    // 如果当前字符是t中需要的
					window[c]++;                         // 窗口内该字符计数加1
					if (window[c] == need[c]) {          // 当该字符出现次数达到需求时
						valid++;                           // 满足的字符种类加1
					}
				}
				// 当窗口已包含t中所有字符（即valid等于need中不同字符的个数）时，尝试收缩窗口
				while (valid == need.size()) {
					if (right - left < len) {            // 更新最小长度
						start = left;
						len = right - left;
					}
					char d = s[left];                     // 即将移出窗口的字符
					left++;                                // 缩小窗口
					if (need.count(d)) {                    // 如果移出的字符是t中需要的
						if (window[d] == need[d]) {          // 移出前该字符刚好满足需求
							valid--;                           // 满足的字符种类减少
						}
						window[d]--;                           // 窗口内该字符计数减1
					}
				}
			}
			return len == INT_MAX ? "" : s.substr(start, len);
		}
		#pragma endregion
		#pragma region 77.组合
		vector<vector<int>> combine(int n, int k) {
			vector<vector<int>> ans;
			if (k == 1)
			{
				for (int i = 1; i <= n; i++)
				{
					ans.push_back({ i });
				}
				return ans;
			}
			if (n >= k)
			{
				vector<vector<int>> bef = combine(n - 1, k);
				vector<vector<int>> add = combine(n - 1, k - 1);
				for (auto nums : bef)
				{
					ans.push_back(nums);
				}
				for (auto nums : add)
				{
					nums.push_back(n);
					ans.push_back(nums);
				}
			}
			return ans;
		}
		#pragma endregion
		#pragma region 78.子集
		void dfs5(vector<vector<int>>& ret, vector<int>& temp,vector<int>& nums,int now)
		{
			if (now == nums.size())
			{
				ret.push_back(temp);
				return;
			}
			temp.push_back(nums[now]);
			dfs5(ret, temp, nums, now + 1);
			temp.pop_back();
			dfs5(ret, temp, nums, now + 1);
		}
		vector<vector<int>> subsets(vector<int>& nums) {
			vector<int> temp;
			vector<vector<int>> ret;
			dfs5(ret, temp, nums, 0);
			return ret;
		}
		#pragma endregion
		#pragma region 79.单词搜索
		vector<pair<int, int>> direction = { {1,0},{-1,0},{0,1},{0,-1} };
		bool check(vector<vector<char>>& board, vector<vector<int>>& visited,string word,int i,int j,int k)
		{
			if (board[i][j] != word[k])return false;
			else if (k == word.length() - 1)return true;
			bool result = false;
			visited[i][j] = 1;
			for (auto p : direction)
			{
				int newi = i + p.first, newj = j + p.second;
				if (newi < board.size() && newi >= 0 && newj < board[0].size() && newj >= 0 && visited[newi][newj] != 1)
				{
					if (check(board, visited, word, newi, newj, k + 1))
					{
						result = true;
						break;
					}
				}
			}
			visited[i][j] = 0;
			return result;
		}
		bool exist(vector<vector<char>>& board, string word) {
			int r = board.size(), l = board[0].size();
			vector<vector<int>> visited(r, vector<int>(l));
			for (int i = 0; i < r; i++)
			{
				for (int j = 0; j < l; j++)
				{
					if (check(board, visited, word, i, j, 0))return true;
				}
			}
			return false;
		}
		#pragma endregion
		#pragma region 80.删除有序数组中重复的相
		int removeDuplicates2(vector<int>& nums) {
			int slow = 1, mid = 1;
			for (int fast = 1; fast < nums.size(); fast++)
			{
				if (nums[fast] != nums[fast - 1])
				{
					if (fast - mid >= 2)
					{
						nums[slow++] = nums[mid];
					}
					mid = fast;
					nums[slow++] = nums[fast];
				}
				else if (fast == nums.size() - 1)
				{
					nums[slow++] = nums[fast];
				}
			}
			return slow;
		}
		#pragma endregion
		#pragma region 81.搜索选择排序数组
		bool search2(vector<int>& nums, int target) {
			if (nums.size() == 0)return false;
			if (nums.size() == 1)return nums[0] == target;
			int left = 0, right = nums.size() - 1;
			while (left <= right)
			{
				int mid = (left + right) / 2;
				if (nums[mid] == target)return true;
				if (nums[left] == nums[mid] && nums[mid] == nums[right])
				{
					left++; right--;
				}
				else if (nums[left] <= nums[mid])
				{
					if (target >= nums[left] && target < nums[mid])
					{
						right = mid - 1;
					}
					else
					{
						left = mid + 1;
					}
				}
				else
				{
					if (target > nums[mid] && target <= nums[right])
					{
						left = mid + 1;
					}
					else
					{
						right = mid - 1;
					}
				}
			}
			return false;
		}
		#pragma endregion
		#pragma region 82.删除排序列表中的重复元素2
		ListNode* deleteDuplicates2(ListNode* head) {
			if (head == NULL)return NULL;
			if (head->next == NULL)return head;
			ListNode* temp = head;
			if (head->val == head->next->val)
			{
				while (temp && temp->val == head->val)
				{
					temp = temp->next;
				}
				return deleteDuplicates(temp);
			}
			head->next = deleteDuplicates(head->next);
			return head;
		}
		#pragma endregion
		#pragma region 83.删除排序列表中的重复元素
		ListNode* deleteDuplicates(ListNode* head) {
			if (head == NULL)return NULL;
			int num = head->val;
			ListNode* slow = head;
			ListNode* fast = head->next;
			while (fast)
			{
				if (fast->val != num)
				{
					slow->next = fast;
					slow = slow->next;
					num = slow->val;
				}
				fast = fast->next;
			}
			slow->next = NULL;
			return head;
		}
		#pragma endregion
		#pragma region 84.柱状图中最大的矩形
		int largestRectangleArea(vector<int>& heights) {
			heights.insert(heights.begin(), 0);
			heights.push_back(0);
			stack<int> s;
			int ret = 0;
			for (int i = 0; i < heights.size(); i++)
			{
				while (!s.empty() && heights[i] < heights[s.top()])
				{
					int h = heights[s.top()];
					s.pop();
					int left = s.empty() ? 0 : s.top();
					int width = i - left - 1;
					int area = h * width;
					ret = max(ret, area);
				}
				s.push(i);
			}
			return ret;
		}
		#pragma endregion
		#pragma region 85.最大矩形
		int maximalRectangle(vector<vector<char>>& matrix) {
			int row = matrix.size();
			int col = matrix[0].size();
			int ret = 0;
			for (int i = 0; i < row; i++)
			{
				for (int j = 0; j < col; j++)
				{

				}
			}
		}
		#pragma endregion
		#pragma region 86.分隔矩形
		ListNode* partition(ListNode* head, int x) {
			ListNode* small = new ListNode(0);
			ListNode* smallHead = small;
			ListNode* big = new ListNode(0);
			ListNode* bigHead = big;
			while (head)
			{
				if (head->val < x)
				{
					small->next = head;
					small = small->next;
				}
				else
				{
					big->next = head;
					big = big->next;
				}
				head = head->next;
			}
			big->next = NULL;
			small->next = bigHead->next;
			return smallHead->next;
		}
		#pragma endregion
		#pragma region 88.合并两个有序数组
		void merge(vector<int>& nums1, int m, vector<int>& nums2, int n) {
			int index1 = m - 1, index2 = n - 1;
			int index = m + n - 1;
			while (index1 >= 0 || index2 >= 0)
			{
				if (index1 == -1)
				{
					nums1[index--] = nums2[index2--];
				}
				else if (index2 == -1)
				{
					nums1[index--] = nums1[index1--];
				}
				else if (nums1[index1] > nums2[index2])
				{
					nums1[index--] = nums1[index1--];
				}
				else
				{
					nums1[index--] = nums2[index2--];
				}
			}
		}
		#pragma endregion
		#pragma region 90.子集2
		void dfs7(vector<vector<int>>&ret,vector<int>& element,vector<int>& nums,int k)
		{
			if (k == nums.size()) 
			{
				ret.push_back(element);
				return;
			}
			element.push_back(nums[k]);
			dfs7(ret, element, nums, k + 1);
			element.pop_back();
			while (k < nums.size() - 1 && nums[k] == nums[k + 1])k++;
			dfs7(ret, element, nums, k + 1);
		}
		vector<vector<int>> subsetsWithDup(vector<int>& nums) {
			vector<vector<int>> ret;
			vector<int> element;
			sort(nums.begin(), nums.end());
			dfs7(ret, element, nums, 0);
			return ret;
		}
		#pragma endregion
		#pragma region 91.解码
		int numDecodings(string s) {
			if (s.empty() || s[0] == '0') return 0;
			int prev2 = 1; 
			int prev1 = 1; 
			for (int i = 2; i <= s.length(); ++i) {
				int cur = 0;
				if (s[i - 1] != '0') {
					cur += prev1;
				}
				int twoDigit = (s[i - 2] - '0') * 10 + (s[i - 1] - '0');
				if (s[i - 2] != '0' && twoDigit <= 26) {
					cur += prev2;
				}
				prev2 = prev1;
				prev1 = cur;
			}
			return prev1;
		}
		#pragma endregion
		#pragma region 92.反转链表2
		ListNode* reverseBetween(ListNode* head, int left, int right) {
			ListNode* dammy = new ListNode(0, head);
			ListNode* slow = dammy;
			for (int i = 1; i < left; i++)
			{
				slow = slow->next;
			}
			ListNode* fast = slow->next;
			for (int i = left; i < right; i++)
			{
				ListNode* temp = fast->next;
				fast->next = temp->next;
				temp->next = slow->next;
				slow->next = temp;
			}
			return dammy->next;
		}
		#pragma endregion
		#pragma region 94.二叉树的中序遍历
		void inorder(TreeNode* root, vector<int>& ans)
		{
			if (root == NULL)return;
			inorder(root->left, ans);
			ans.push_back(root->val);
			inorder(root->right, ans);
		}
		vector<int> inorderTraversal(TreeNode* root) {
			vector<int> ans;
			inorder(root, ans);
			return ans;
		}
		#pragma endregion
		#pragma region 95.不同的二叉搜索树2
		vector<TreeNode*> generateTrees(int start, int end)
		{
			if (start > end)return{};
			vector<TreeNode*> allTrees;
			for (int i = start; i <= end; i++)
			{
				vector<TreeNode*> leftTrees = generateTrees(start, i - 1);
				vector<TreeNode*> rightTrees = generateTrees(i + 1, end);
				for (auto left : leftTrees)
				{
					for (auto right : rightTrees)
					{
						TreeNode* tree = new TreeNode(i);
						tree->left = left;
						tree->right = right;
						allTrees.push_back(tree);
					}
				}
			}
			return allTrees;
		}
		vector<TreeNode*> generateTrees(int n) {
			if (n <= 0)return {};
			return generateTrees(1, n);
		}
		#pragma endregion
		#pragma region 96.不同的二叉搜索树
		//int numTrees(int start, int end)
		//{
		//	if (start > end)return 1;
		//	int ret = 0;
		//	for (int i = start; i <= end; i++)
		//	{
		//		int numleft = numTrees(start, i - 1);
		//		int numright = numTrees(i + 1, end);
		//		ret += (numleft * numright);
		//	}
		//	return ret;
		//}
		//int numTrees(int n) {
		//	if (n <= 0)return 0;
		//	return numTrees(1, n);
		//}
		int numTrees(int n) {
			vector<int> dp(n + 1);
			dp[0] = 1, dp[1] = 1;
			for (int i = 2; i < n + 1; i++)
			{
				for (int j = 0; j < i; j++)
				{
					dp[i] += dp[j] * dp[i - 1 - j];
				}
			}
			return dp[n];
		}
		#pragma endregion
		#pragma region 97.交错字符串
		bool isInterleave(string s1, string s2, string s3) {
			int m = s1.length(), n = s2.length();
			if (m + n != s3.length())return false;
			vector<vector<bool>> dp(m + 1, vector<bool>(n + 1, false));
			dp[0][0] = true;
			for (int i = 0; i <= m; i++)
			{
				for (int j = 0; j <= n; j++)
				{
					if (i > 0 && s3[i + j - 1] == s1[i - 1] && dp[i - 1][j])
					{
						dp[i][j] = true;
					}
					if (j > 0 && s3[i + j - 1] == s2[j - 1] && dp[i][j - 1])
					{
						dp[i][j] = true;
					}
				}
			}
			return dp[m][n];
		}
		#pragma endregion
		#pragma region 98.验证二叉搜索树
		bool isValidBST(TreeNode* root, long long min, long long max)
		{
			if (root == NULL) return true;
			if (root->val <= min || root->val >= max)return false;
			return isValidBST(root->left, min, root->val) && isValidBST(root->right, root->val, max);
		}
		bool isValidBST(TreeNode* root) {
			return isValidBST(root, LLONG_MIN, LLONG_MAX);
		}
		#pragma endregion
		#pragma region 99.恢复二叉搜索树
		//TreeNode* findMax(TreeNode* root) 
		//{
		//	if (root == NULL) return NULL;
		//	TreeNode* leftMax = findMax(root->left);
		//	TreeNode* rightMax = findMax(root->right);
		//	TreeNode* maxNode = root;
		//	if (leftMax && leftMax->val > maxNode->val) maxNode = leftMax;
		//	if (rightMax && rightMax->val > maxNode->val) maxNode = rightMax;
		//	return maxNode;
		//}
		//TreeNode* findMin(TreeNode* root)
		//{
		//	if (root == NULL)return NULL;
		//	TreeNode* leftMin = findMin(root->left);
		//	TreeNode* rightMin = findMin(root->right);
		//	TreeNode* minNode = root;
		//	if (leftMin && leftMin->val < minNode->val)minNode = leftMin;
		//	if (rightMin && rightMin->val < minNode->val)minNode = rightMin;
		//	return minNode;
		//}
		//void recoverTree(TreeNode* root) {
		//	if (root == NULL)return;
		//	TreeNode* leftMax = findMax(root->left);
		//	TreeNode* rightMin = findMin(root->right);
		//	if (leftMax && root->val < leftMax->val)
		//	{
		//		swap(root->val, leftMax->val);
		//		return;
		//	}
		//	if (rightMin && root->val > rightMin->val)
		//	{
		//		swap(root->val, rightMin->val);
		//		return;
		//	}
		//	recoverTree(root->left);
		//	recoverTree(root->right);
		//}
		void recoverTree(TreeNode* root) {
			TreeNode* first = nullptr, * second = nullptr, * prev = nullptr;
			inorder(root, first, second, prev);
			swap(first->val, second->val);
		}
		void inorder(TreeNode* node, TreeNode*& first, TreeNode*& second, TreeNode*& prev) {
			if (!node) return;
			inorder(node->left, first, second, prev);
			if (prev && prev->val > node->val) {
				if (!first) first = prev;
				second = node;
			}
			prev = node;
			inorder(node->right, first, second, prev);
		}
		#pragma endregion
		#pragma region 100.相同的树
		bool isSameTree(TreeNode* p, TreeNode* q) {
			if (p == NULL && q == NULL)return true;
			if (p != NULL && q != NULL)
			{
				if (p->val == q->val)
				{
					return isSameTree(p->left, q->left) && isSameTree(p->right, q->right);
				}
				return false;
			}
			return false;
		}
		#pragma endregion
		#pragma region 101.对称二叉树
		bool check(TreeNode* p, TreeNode* q) {
			if (p == NULL && q == NULL)return true;
			if (p == NULL || q == NULL)return false;
			return (p->val == q->val) && check(p->left, q->right) && check(p->right, q->left);
		}
		bool isSymmetric(TreeNode* root) {
			if (root == NULL)return true;
			return check(root->left, root->right);
		}
		#pragma endregion
		#pragma region 102.二叉树的层序遍历
		vector<vector<int>> levelOrder(TreeNode* root) {
			if (root == NULL)return{};
			queue<TreeNode*> trees;
			TreeNode* last = root;
			TreeNode* nlast = NULL;
			trees.push(root);
			vector<vector<int>> ret;
			vector<int> temp;
			while (!trees.empty())
			{
				TreeNode* node = trees.front();
				temp.push_back(node->val);
				trees.pop();
				if (node->left)
				{
					nlast = node->left;
					trees.push(node->left);
				}
				if (node->right)
				{
					nlast = node->right;
					trees.push(node->right);
				}
				if (node == last)
				{
					ret.push_back(temp);
					temp.clear();
					last = nlast;
				}
			}
			return ret;
		}
		#pragma endregion
		#pragma region 103.二叉树的锯齿形层序遍历
		vector<vector<int>> zigzagLevelOrder(TreeNode* root) {
			if (root == NULL)return {};
			queue<TreeNode*> nodes;
			nodes.push(root);
			vector<vector<int>> ret;
			bool isOrderleft = true;
			while (!nodes.empty()) 
			{
				deque<int> valList;
				int size = nodes.size();
				for (int i = 0; i < size; i++)
				{
					TreeNode* node = nodes.front();
					if (isOrderleft)
					{
						valList.push_back(node->val);
					}
					else
					{
						valList.push_front(node->val);
					}
					nodes.pop();
					if (node->left)nodes.push(node->left);
					if (node->right)nodes.push(node->right);
				}
				ret.push_back(vector<int>{valList.begin(), valList.end()});
				isOrderleft = !isOrderleft;
			}
			return ret;
		}
		#pragma endregion
		#pragma region 104.二叉树的最大深度
		int maxDepth(TreeNode* root) {
			if (root == NULL)return 0;
			else return (1 + max(maxDepth(root->left), maxDepth(root->right)));
		}
		#pragma endregion

};
int main()
{
	Solution solution;
#pragma region 1.两数之和
	{
		vector<int> nums = { 2, 7, 11, 15 };
		int target = 9;
		vector<int> result = solution.twoSum(nums, target);
		for (int index : result)
		{
			cout << index << " ";
		}
		cout << endl;
	}
#pragma endregion
#pragma region 2.两数相加
	{
		ListNode* l1 = new ListNode(8, new ListNode(6, new ListNode(5)));
		ListNode* l2 = new ListNode(2, new ListNode(3, new ListNode(4)));
		ListNode* ret = solution.addTwoNumbers(l1, l2);
		while (ret)
		{
			cout << ret->val;
			ret = ret->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 3.无重复字符的最长字串
	{
		int maxLength = solution.lengthOfLongestSubstring("abcacbb");
		cout << maxLength << endl;
	}
#pragma endregion
#pragma region 4.寻找两个正序数组的中位数
	{
		vector<int> nums1 = { 1,2,4 };
		vector<int> nums2 = { 2,5,6 };
		double med = solution.findMedianSortedArrays(nums1, nums2);
		cout << med << endl;
	}
#pragma endregion
#pragma region 5.最长回文字串
	{
		string palindrome = solution.longestPalindrome("bbcabacb");
		cout << palindrome << endl;
	}
#pragma endregion
#pragma region 6.Z字变换
	{
		string convertRet = solution.convert("PAYPALISHIRING", 3);
		cout << convertRet << endl;
	}
#pragma endregion
//#pragma region 7.整数反转
//	{
//		int reverse1 = solution.reverse(123);
//		int reverse2 = solution.reverse(INT32_MAX);
//		cout << reverse1 << " " << reverse2 << endl;
//	}
//#pragma endregion
#pragma region 8.atoi
	{
		int atoi = solution.myAtoi("as-0123as");
		cout << atoi << endl;
	}
#pragma endregion
#pragma region 9.回文数
	{
		cout << solution.isPalindrome(12321) << endl;
	}
#pragma endregion
#pragma region 10.正则表达式匹配
	{
		cout << solution.isMatch("aab", "c*a*b") << endl;
	}
#pragma endregion
#pragma region 11.装水最多的容器
	{
		vector<int> height = { 1,8,6,2,5,4,8,3,7 };
		cout << solution.maxArea(height) << endl;
	}
#pragma endregion
#pragma region 12.整数转罗马数字
	{
		cout << solution.intToRoman(3999) << endl;
	}
#pragma endregion
#pragma region 13.罗马数字转整数
	{
		cout << solution.romanToInt("MCMXCIV") << endl;
	}
#pragma endregion
#pragma region 14.最长公共前缀
	{
		vector<string> strs = { "flower","flow","flight" };
		cout << solution.longestCommonPrefix(strs) << endl;
	}
#pragma endregion
#pragma region 15.三数之和
	{
		vector<int> nums = { -1,0,1,2,-1,-4 };
		vector<vector<int>> ret = solution.threeSum(nums);
		for (auto array : ret)
		{
			for (auto num : array)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 16.最接近的三数之和
	{
		vector<int> nums = { -1,2,1,-4 };
		int ret = solution.threeSumClosest(nums, 1);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 17.电话号码的字母组合
	{
		vector<string> ret = solution.letterCombinations("23");
		for (auto s : ret)
		{
			cout << s << " " ;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 18.四数之和
	{
		vector<int> nums = { -5,-4,4,5 };
		vector<vector<int>> ret = solution.fourSum(nums, 0);
		for (auto ints : ret)
		{
			for (auto num : ints)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 19.删除链表的倒数第N个结点
	{
		ListNode* l1 = new ListNode(1, new ListNode(2, new ListNode(3,new ListNode(4,new ListNode(5)))));
		ListNode* ret = solution.removeNthFromEnd(l1, 4);
		while (ret)
		{
			cout << ret->val << " ";
			ret = ret->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 20.有效的括号
	{
		cout << solution.isValid("") << endl;
	}
#pragma endregion
#pragma region 21.合并两个有序链表
	{
		ListNode* l1 = new ListNode(1, new ListNode(2, new ListNode(5)));
		ListNode* l2 = new ListNode(0, new ListNode(3, new ListNode(4)));
		ListNode* ret = solution.mergeTwoLists(l1, l2);
		while (ret)
		{
			cout << ret->val << " ";
			ret = ret->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 22.括号生成
	{
		vector<string> ret = solution.generateParenthesis(3);
		for (string s : ret)
		{
			cout << s << "	";
		}
		cout<<endl;
	}
#pragma endregion
#pragma region 23.合并K个升序链表
	{
		vector<ListNode*> lists = { new ListNode(1,new ListNode(4,new ListNode(5))),
									new ListNode(1,new ListNode(3,new ListNode(4))),
									new ListNode(2,new ListNode(6)) };
		ListNode* ret = solution.mergeKLists(lists);
		while (ret)
		{
			cout << ret->val << " ";
			ret = ret->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 24.两两交换链表中的节点
	{
		ListNode* list = new ListNode(1, new ListNode(2, new ListNode(3, new ListNode(4, new ListNode(5,new ListNode(6))))));
		ListNode* ret = solution.swapPairs(list);
		while (ret)
		{
			cout << ret->val << " ";
			ret = ret->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 25.K个一组翻转链表
	{
		ListNode* list = new ListNode(1, new ListNode(2, new ListNode(3, new ListNode(4, new ListNode(5,new ListNode(6))))));
		ListNode* ret = solution.reverseKGroup(list, 2);
		while (ret)
		{
			cout << ret->val << " ";
			ret = ret->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 26.删除有序数组中的重复项
	{
		vector<int> nums = { 1,2,3,3,3,4,4,4,5,5,6,7 };
		int ret = solution.removeDuplicates2(nums);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 27.移除元素
	{
		vector<int> nums = { 1,2,3,3,3,4,4,5,6,7 };
		int ret = solution.removeElement(nums, 3);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 28.找出字符串中第一个匹配的下标
	{
		string s1 = "sadbutsad";
		string s2 = "ad";
		int ret = solution.strStr(s1, s2);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 29.两数相除
	{
		cout << solution.divide(-2147483648, 2) << endl;
	}
#pragma endregion
#pragma region 30.串联所以单词的子串
	{
		string s = "barfoothefoobarman";
		vector<string> words = { "bar","foo" };
		vector<int> ret = solution.findSubstring(s, words);
		for (int num : ret)
		{
			cout << num << " ";
		}
		cout << endl;
	}
#pragma endregion
#pragma region 31.下一个排列
	{
		vector<int> nums = { 1,1,5 };
		solution.nextPermutation(nums);
		for (int num : nums)
		{
			cout << num << " ";
		}
		cout << endl;
	}
#pragma endregion
#pragma region 32.最长有效括号
	{
		cout << solution.longestValidParentheses("") << endl;;
	}
#pragma endregion
#pragma region 33.搜索旋转排序数组
	{
		vector<int> nums = { 4,5,6,7,0,1,2 };
		int ret = solution.search(nums, 0);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 38.外观数列
	{
		string ret = solution.countAndSay(5);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 41.缺失的第一个正数
	{
		vector<int> nums = { 1,1,2,2,0 };
		int ret = solution.firstMissingPositive(nums);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 42.接雨水
	{
		vector<int> height = { 0,1,0,2,1,0,1,3,2,1,2,1 };
		int ret = solution.trap(height);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 43.字符串相乘
	{
		string num1 = "103";
		string num2 = "22";
		string ret = solution.multiply(num1, num2);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 45.跳跃游戏2
	{
		vector<int> nums = { 2,0,3,1,4,1,1 };
		int ret = solution.jump(nums);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 46.全排序
	{
		vector<int> nums = { 1,2,3 };
		vector<vector<int>> ret = solution.permute(nums);
		for (auto array : ret)
		{
			for (auto num : array)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 47.全排序2
	{
		vector<int> nums = { 0,0,1,9 };
		vector<vector<int>> ret = solution.permuteUnique(nums);
		for (auto array : ret)
		{
			for (auto num : array)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 54.螺旋数组
	{
		vector<vector<int>> nums = { {1,2,3},{4,5,6},{7,8,9} };
		vector<int>ret = solution.spiralOrder(nums);
		for (auto num : ret)cout << num << " ";
		cout << endl;
	}
#pragma endregion
#pragma region 55,跳跃游戏
	{
		vector<int> nums = { 3,2,1,0,4 };
		cout << solution.canJump(nums) << endl;
	}
#pragma endregion
#pragma region 56.合并区间
	{
		vector<vector<int>> intervals = { {1,3},{2,6},{8,10},{15,18} };
		vector<vector<int>> merged = solution.merge(intervals);
		for (auto nums : merged)
		{
			for (int num : nums)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 58.最后一个单词长度
	{
		string s = "a";
		int ret = solution.lengthOfLastWord(s);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 59.螺旋矩阵
	{
		vector<vector<int>> ret = solution.generateMatrix(4);
		for (auto arr : ret)
		{
			for (int num : arr)
			{
				cout << num << ' ';
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 61.旋转链表
	{
		ListNode* list = new ListNode(1, new ListNode(2, new ListNode(3, new ListNode(4, new ListNode(5)))));
		ListNode* ret = solution.rotateRight(list, 2);
		while (ret)
		{
			cout << ret->val << " ";
			ret = ret->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 62.不同路径
	{
		int ret = solution.uniquePaths(3, 7);
		cout << ret << endl;
	}
#pragma endregion
#pragma region 63.不同路径
	{
		vector<vector<int>> obstacles = { {0,0,1} };
		cout << solution.uniquePathsWithObstacles(obstacles) << endl;
	}
#pragma endregion
#pragma region 64.最小路径和
	{
		vector<vector<int>> path = { {1,2,3} ,{4,5,6} };
		cout << solution.minPathSum(path) << endl;
	}
#pragma endregion
#pragma region 66.加一
	{
		vector<int> digits = { 9,9,8 };
		vector<int> ret = solution.plusOne(digits);
		for (int num : ret)
		{
			cout << num;
		}
		cout<<endl;
	}
#pragma endregion
#pragma region 68.文本左右对齐
	{
		vector<string> words = { "What","must","be","acknowledgment","shall","be" };
		vector<string> ret = solution.fullJustify(words,16);
		for (auto s : ret)
		{
			cout << s << endl;
		}
	}
#pragma endregion
#pragma region 73.矩阵置零
	{
		vector<vector<int>> matrix = { {1,1,1},{1,0,1},{1,1,1} };
		solution.setZeroes(matrix);
		for (auto row : matrix)
		{
			for (auto num : row)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 76.最小覆盖子串
	{
		string s = "ADOBECODEBANC", t = "ABC";
		cout << solution.minWindow(s, t) << endl;
	}
#pragma endregion
#pragma region 77.组合
	{
		vector<vector<int>> ret = solution.combine(5, 2);
		for (auto nums : ret)
		{
			for (auto num : nums) 
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 78.子集
	{
		vector<vector<int>> ans;
		vector<int> nums = { 1,2,3 };
		ans = solution.subsets(nums);
		for (auto nums : ans)
		{
			for (auto num : nums)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion
#pragma region 84.柱状图中最大的矩形
	{
		vector<int> heights = { 2,1,2 };
		int maxSquare = solution.largestRectangleArea(heights);
		cout << maxSquare << endl;
	}
#pragma endregion
#pragma region 86.分隔链表
	{
		ListNode* list = new ListNode(1, new ListNode(4, new ListNode(3, new ListNode(5, new ListNode(2)))));
		ListNode* ret = solution.partition(list, 3);
		while (list)
		{
			cout << list->val << " ";
			list = list->next;
		}
		cout << endl;
	}
#pragma endregion
#pragma region 90.子集2
	{
		vector<int>nums = { 1,2,2 };
		vector<vector<int>> ans = solution.subsetsWithDup(nums);
		for (auto nums : ans)
		{
			for (auto num : nums)
			{
				cout << num << " ";
			}
			cout << endl;
		}
	}
#pragma endregion

	return 0;
}
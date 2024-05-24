#include "bucket_storage.hpp"
#include "iostream"
#include "stack.hpp"

#include <gtest/gtest.h>

#include <random>

#define LOGGING 0
void p()
{
#if LOGGING
	std::cout << actions.back() << '\n';
#endif
}

// Test int wrapper class for examining lvalue and rvalue correctness
struct S
{
	static std::vector< const char * > actions;

	constexpr static const char *const CONSTRUCTOR = "S constructor";
	constexpr static const char *const DESTRUCTOR = "S destructor";
	constexpr static const char *const LVALUE_COPY_CONSTRUCTOR = "S lvalue copy constructor";
	constexpr static const char *const RVALUE_COPY_CONSTRUCTOR = "S rvalue copy constructor";
	constexpr static const char *const RVALUE_ASSIGNMENT_OPERATOR = "S rvalue assignment operator";
	constexpr static const char *const LVALUE_ASSIGNMENT_OPERATOR = "S lvalue assignment operator";

	explicit S(int i) : x(i)
	{
		actions.push_back(CONSTRUCTOR);
		p();
	}
	S(const S &s)
	{
		x = s.x;
		actions.push_back(LVALUE_COPY_CONSTRUCTOR);
		p();
	}
	S &operator=(const S &s)
	{
		x = s.x;
		actions.push_back(LVALUE_ASSIGNMENT_OPERATOR);
		p();
		return *this;
	}
	S(S &&s)
	{
		x = s.x;
		actions.push_back(RVALUE_COPY_CONSTRUCTOR);
		p();
	}
	S &operator=(S &&s)
	{
		x = s.x;
		actions.push_back(RVALUE_ASSIGNMENT_OPERATOR);
		p();
		return *this;
	}
	~S()
	{
		actions.push_back(DESTRUCTOR);
		p();
	}

	friend std::ostream &operator<<(std::ostream &os, const S &s)
	{
		os << s.x;
		return os;
	}
	bool operator<(const S &other) const { return x < other.x; }
	bool operator==(const S &other) const { return x == other.x; }

	int x;
};
std::vector< const char * > S::actions;

TEST(stack, pushpop)
{
	Stack< int > stack;
	stack.push(1);
	stack.push(2);
	stack.push(3);
	EXPECT_EQ(stack.pop(), 3);
	EXPECT_EQ(stack.pop(), 2);
	stack.push(4);
	stack.push(5);
	EXPECT_EQ(stack.pop(), 5);
	EXPECT_EQ(stack.pop(), 4);
	EXPECT_EQ(stack.pop(), 1);
	stack.push(6);
	stack.push(7);
}

TEST(bucket_storage, rvalue_insert_erase)
{
	S::actions.clear();
	BucketStorage< S > ss(3);
	for (int i = 1; i <= 10; i++)
	{
		ss.insert(S(i));
		EXPECT_EQ(S::actions[S::actions.size() - 2], S::RVALUE_COPY_CONSTRUCTOR);
	}
	{
		int i = 0;
		for (auto &s : ss)
		{
			EXPECT_EQ(s.x, ++i);
		}
	}
	{
		auto it = ss.end();
		int i = 10;
		while (!ss.empty())
		{
			--it;
			EXPECT_EQ((*it).x, i--);
			it = ss.erase(it);
		}
	}
}
TEST(bucket_storage, lvalue_insert)
{
	S::actions.clear();
	BucketStorage< S > ss(3);
	for (int i = 1; i <= 10; i++)
	{
		S s(i);
		ss.insert(s);
		EXPECT_EQ(S::actions.back(), S::LVALUE_COPY_CONSTRUCTOR);
	}
	int i = 0;
	for (auto &s : ss)
	{
		EXPECT_EQ(s.x, ++i);
	}
}

// randomly chooses to insert or delete an S element
// control vector is used to verify operations' correctness
// the check is done via getting all BS elements into a vector and comparing sorted data
TEST(bucket_storage, random)
{
	std::random_device dev;
	std::random_device::result_type seed = dev();	 // you can use your seed to get the same inputs every time
	std::mt19937 rng(seed);

	const int iterations = 120;		   // How many times to attempt insert()/erase()
	const double delete_prob = 0.1;	   // Probability for erase()

	BucketStorage< S > bs;	  // Here you can call an alternative constructor
	std::vector< S > v;
	auto insert = [&](int x)
	{
		v.emplace_back(x);
		bs.insert(S(x));
	};
	auto check = [&]()
	{
		for (auto &s : bs)
		{
			std::cout << s << ' ';
		}
		std::cout << '\n';
		EXPECT_EQ(bs.size(), v.size());
		std::vector< S > bs_data;
		for (auto &s : bs)
		{
			bs_data.push_back(s);
		}
		std::sort(bs_data.begin(), bs_data.end());
		std::sort(v.begin(), v.end());
		EXPECT_EQ(bs_data, v);
	};
	auto erase = [&](BucketStorage< S >::const_iterator bsit, std::vector< S >::const_iterator vit)
	{
		return std::make_pair(bs.erase(bsit), v.erase(vit));
	};

	std::uniform_real_distribution<> prob(0.0, 1.0);
	std::uniform_int_distribution<> ints(0, std::numeric_limits< int >::max());
	for (int i = 0; i < iterations; i++)
	{
		double r = prob(rng);
		if (r <= delete_prob)
		{
			if (bs.size() == 0)
				continue;
			std::uniform_int_distribution<> d(0, (int)v.size() - 1);
			int index = d(rng);
			auto vit = v.begin() + index;
			auto bsit = std::find(bs.begin(), bs.end(), v[index]);
			EXPECT_NE(bsit, bs.end());
			std::cout << "deleting: " << v[index] << '\n';
			erase(bsit, vit);
			check();
		}
		else
		{
			int to_insert = ints(rng);
			insert(to_insert);
			std::cout << "inserting: " << to_insert << '\n';
			check();
		}
	}
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

#include "bucket_storage.hpp"
#include "iostream"

#include <gtest/gtest.h>

#include <random>
#include <utility>

#define S_OP_LOGGING 0		 // whether to log S actions
#define RANDOM_TEST_LOG 0	 // whether to print BS state in random test
#define STACK_TEST 0		 // enable Stack<T> test

/* Warning: tests marked with 'assumes insertion order'
 * test for specific order in BucketStorage, that is,
 * the iterator should return elements in
 * the order in which they were inserted (without erase)
 */

void p()
{
#if S_OP_LOGGING
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
	S(S &&s) noexcept
	{
		x = s.x;
		actions.push_back(RVALUE_COPY_CONSTRUCTOR);
		p();
	}
	S &operator=(S &&s) noexcept
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

#if STACK_TEST
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
#endif

void print(BucketStorage< S > &bs)
{
	for (auto &s : bs)
	{
		std::cout << s << ' ';
	}
	std::cout << '\n';
}
// randomly chooses to insert or delete an S element
// control vector is used to verify operations' correctness
// the check is done by getting all BS elements into a vector and comparing sorted data
TEST(bucket_storage, random)
{
	std::random_device dev;
	std::random_device::result_type seed = dev();	 // you can use your seed to get the same inputs every time
	std::mt19937 rng(seed);

	const int iterations = 200;		   // How many times to attempt insert()/erase()
	const double delete_prob = 0.1;	   // Probability for erase()

	BucketStorage< S > bs(20);	  // Here you can call an alternative constructor
	std::vector< S > v;
	auto insert = [&](int x)
	{
		v.emplace_back(x);
		bs.insert(S(x));
	};
	auto check = [&]()
	{
#if RANDOM_TEST_LOG
		print(bs);
#endif
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
			if (v.size() == 0)
				continue;
			std::uniform_int_distribution<> d(0, (int)v.size() - 1);
			int index = d(rng);
			auto vit = v.begin() + index;
			auto bsit = std::find(bs.begin(), bs.end(), v[index]);	  // requires std::iterator_traits
			EXPECT_NE(bsit, bs.end());
			EXPECT_EQ(*bsit, v[index]);
#if RANDOM_TEST_LOG
			std::cout << "deleting: " << v[index] << '\n';
#endif
			erase(bsit, vit);
			check();
		}
		else
		{
			int to_insert = ints(rng);
			insert(to_insert);
#if RANDOM_TEST_LOG
			std::cout << "inserting: " << to_insert << '\n';
#endif
			check();
		}
	}
}
TEST(bucket_storage, swap)
{
	/* Fun fact:
	 * std::swap can (and is implemented in standard library by default)
	 * in terms of move assignment and move construction
	 * like you would do
	 * int x, y;
	 * int t = x;
	 * x = y;
	 * y = t;
	 */
	BucketStorage< S > bs1;
	bs1.insert(S(1));
	BucketStorage< S > bs2(20);
	bs2.insert(S(2));

	int before = bs1.begin()->x;
	bs1.swap(bs2);
	EXPECT_EQ(bs2.begin()->x, before);
	bs1.swap(bs2);
	EXPECT_EQ(bs1.begin()->x, before);
}

// assumes insertion order
TEST(assuming_order, rvalue_insert_erase)
{
	BucketStorage< S > ss(3);
	for (int i = 1; i <= 10; i++)
	{
		ss.insert(S(i));
		EXPECT_EQ(S::actions[S::actions.size() - 2], S::RVALUE_COPY_CONSTRUCTOR)
			<< "rvalue copy constructor should be "
			   "used\n";
	}
	{
		int i = 0;
		for (auto &s : ss)
		{
			EXPECT_EQ(s.x, ++i) << "incorrect value\n";
		}
	}
	{
		auto it = ss.end();
		int i = 10;
		while (!ss.empty())
		{
			--it;
			EXPECT_EQ(it->x, i--) << "incorrect value\n";
			it = ss.erase(it);
		}
	}
}

// assumes insertion order
TEST(assuming_order, lvalue_insert)
{
	BucketStorage< S > ss(3);
	for (int i = 1; i <= 10; i++)
	{
		S s(i);
		ss.insert(s);
		EXPECT_EQ(S::actions.back(), S::LVALUE_COPY_CONSTRUCTOR) << "expected lvalue copy constructor\n";
	}
	int i = 0;
	for (auto &s : ss)
	{
		EXPECT_EQ(s.x, ++i) << "incorrect value\n";
	}
}

// assumes insertion order
TEST(assuming_order, RAII)
{
	BucketStorage< S > bs_lvalue;
	bs_lvalue.insert(S(1));
	bs_lvalue.insert(S(2));

	bs_lvalue = BucketStorage< S >(2);	  // rvalue assignment
	bs_lvalue.insert(S(3));
	bs_lvalue.insert(S(4));
	bs_lvalue.insert(S(5));
	EXPECT_EQ(bs_lvalue.size(), 3);
	EXPECT_EQ(bs_lvalue.capacity(), 4);
	EXPECT_EQ(bs_lvalue.begin()->x, 3);
	EXPECT_EQ((++(++bs_lvalue.begin()))->x, 5);

	BucketStorage< S > bs_copy(bs_lvalue);	  // lvalue copy constructor
	EXPECT_EQ(bs_copy.size(), 3);
	EXPECT_EQ(bs_copy.capacity(), 4);
	EXPECT_EQ(bs_copy.begin()->x, 3);
	EXPECT_EQ((++(++bs_lvalue.begin()))->x, 5);

	bs_lvalue = bs_lvalue;	  // self assignment
	EXPECT_EQ(bs_lvalue.size(), 3);
	EXPECT_EQ(bs_lvalue.capacity(), 4);
	EXPECT_EQ(bs_lvalue.begin()->x, 3);
	EXPECT_EQ((++(++bs_lvalue.begin()))->x, 5);

	BucketStorage< S > bs_rvalue_constructed(BucketStorage< S >(20));
	bs_rvalue_constructed.insert(S(1));
	EXPECT_EQ(bs_rvalue_constructed.size(), 1);
	EXPECT_EQ(bs_rvalue_constructed.capacity(), 20);
	EXPECT_EQ(bs_rvalue_constructed.begin()->x, 1);
}

// assumes insertion order
TEST(assuming_order, block_capacity_extremes2)
{
	BucketStorage< S > ss(2);
	EXPECT_EQ(ss.insert(S(1))->x, 1);
	EXPECT_EQ(ss.insert(S(2))->x, 2);
	EXPECT_EQ(ss.insert(S(3))->x, 3);

	auto it = ss.begin();
	EXPECT_EQ((it++)->x, 1);
	EXPECT_EQ((it++)->x, 2);
	EXPECT_EQ((it++)->x, 3);
}

// assumes insertion order
TEST(assuming_order, block_capacity_extremes1)
{
	BucketStorage< S > ss1(1);
	EXPECT_EQ(ss1.insert(S(1))->x, 1);
	EXPECT_EQ(ss1.insert(S(2))->x, 2);
	EXPECT_EQ(ss1.insert(S(3))->x, 3);

	auto it = ss1.begin();
	EXPECT_EQ((it++)->x, 1);
	EXPECT_EQ((it++)->x, 2);
	EXPECT_EQ((it++)->x, 3);
	EXPECT_EQ(it, ss1.end());
}

TEST(bucket_storage, const_bs)
{
	const BucketStorage< S > const_bs;
	for (auto &s : const_bs)
	{
		(void)s;
	}
	BucketStorage< S > bs = const_bs;
	bs.insert(S(1));
	const auto it = bs.begin();
	EXPECT_EQ(it->x, 1);
}

template< class ContainerType >
concept Container = requires(ContainerType a, const ContainerType b) {
	requires std::regular< ContainerType >;
	requires std::swappable< ContainerType >;
	requires std::destructible< typename ContainerType::value_type >;
	requires std::same_as< typename ContainerType::reference, typename ContainerType::value_type & >;
	requires std::same_as< typename ContainerType::const_reference, const typename ContainerType::value_type & >;
	requires std::forward_iterator< typename ContainerType::iterator >;
	requires std::forward_iterator< typename ContainerType::const_iterator >;
	requires std::signed_integral< typename ContainerType::difference_type >;
	requires std::same_as< typename ContainerType::difference_type, typename std::iterator_traits< typename ContainerType::iterator >::difference_type >;
	requires std::same_as< typename ContainerType::difference_type, typename std::iterator_traits< typename ContainerType::const_iterator >::difference_type >;
	{
		a.begin()
	} -> std::same_as< typename ContainerType::iterator >;
	{
		a.end()
	} -> std::same_as< typename ContainerType::iterator >;
	{
		b.begin()
	} -> std::same_as< typename ContainerType::const_iterator >;
	{
		b.end()
	} -> std::same_as< typename ContainerType::const_iterator >;
	{
		a.cbegin()
	} -> std::same_as< typename ContainerType::const_iterator >;
	{
		a.cend()
	} -> std::same_as< typename ContainerType::const_iterator >;
	{
		a.size()
	} -> std::same_as< typename ContainerType::size_type >;
	/*
	{
		// maybe we don't really need this
		// unless you find a sensible way to calculate this
		a.max_size()
	} -> std::same_as< typename ContainerType::size_type >;
	*/
	{
		a.empty()
	} -> std::same_as< bool >;
};
template< Container C >
void container_f(C c)
{
	(void)c;
}

TEST(bucket_storage, concepts)
{
	EXPECT_TRUE(std::is_default_constructible< BucketStorage< S > >());
	EXPECT_TRUE(std::is_copy_assignable< BucketStorage< S > >());
	EXPECT_TRUE(std::is_move_assignable< BucketStorage< S > >());
	EXPECT_TRUE(std::copy_constructible< BucketStorage< S > >);
	EXPECT_TRUE(std::move_constructible< BucketStorage< S > >);
	EXPECT_TRUE(std::destructible< BucketStorage< S > >);

	EXPECT_TRUE(Container< BucketStorage< S > >);
	// container_f(BucketStorage< S >());	  // uncomment to see why it's failing
}

class TraceHandler : public testing::EmptyTestEventListener
{
	// Called after a test ends.
	void OnTestEnd(const testing::TestInfo &test_info) override
	{
		(void)test_info;
		S::actions.clear();
	}
};

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);

	testing::TestEventListeners &listeners = testing::UnitTest::GetInstance()->listeners();
	listeners.Append(new TraceHandler);

	return RUN_ALL_TESTS();
}

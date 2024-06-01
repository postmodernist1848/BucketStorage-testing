#include "bucket_storage.hpp"
#include "iostream"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <random>
#include <utility>
#include <vector>

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

// memory allocating object for memory leak testing
class M
{
  public:
	explicit M(int n) : data(new int[size])
	{
		for (size_t i = 0; i < size; i++)
		{
			data[i] = n;
		}
	}
	M(const M &m) : data(new int[size]) { memcpy(data, m.data, size * sizeof *data); }
	M(M &&m) noexcept : data(nullptr) { std::swap(data, m.data); }
	M &operator=(M m)
	{
		std::swap(data, m.data);
		return *this;
	}
	~M() { delete[] data; }

	friend std::ostream &operator<<(std::ostream &os, const M &m)
	{
		os << '[' << m.data[0] << ", ..., " << m.data[size - 1] << ']';
		return os;
	}

  private:
	static constexpr size_t size = 32;
	int *data;
};

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

std::vector< S > to_sorted_vector(BucketStorage< S > &bs)
{
	std::vector< S > bs_data;
	for (auto &s : bs)
	{
		bs_data.push_back(s);
	}
	std::sort(bs_data.begin(), bs_data.end());
	return bs_data;
}

// basically checks if multiset of lhs is the same as rhs
void expect_same_elements(BucketStorage< S > &lhs, BucketStorage< S > &rhs)
{
	EXPECT_EQ(to_sorted_vector(lhs), to_sorted_vector(rhs));
}

// modifies vector
void expect_same_elements(std::vector< S > &lhs, BucketStorage< S > &rhs)
{
	std::sort(lhs.begin(), lhs.end());
	EXPECT_EQ(lhs, to_sorted_vector(rhs));
}
void expect_same_elements(BucketStorage< S > &lhs, std::vector< S > &rhs)
{
	expect_same_elements(rhs, lhs);
}

std::random_device dev;
std::random_device::result_type seed = dev();	 // you can use your seed to get the same inputs every time
std::mt19937 rng(seed);

double randdouble()
{
	static std::uniform_real_distribution<> prob(0.0, 1.0);
	return prob(rng);
}
int randint(int low, int high)
{
	std::uniform_int_distribution<> dist(low, high);
	return dist(rng);
}

class Id
{
	static int id;

  public:
	static int get_id() { return ++id; }
	static void reset() { id = 0; }
};
int Id::id = 0;

void insert(BucketStorage< S > &bs, std::vector< S > &v, int x)
{
	v.emplace_back(x);
	bs.insert(S(x));
}

// randomly chooses to insert or delete an S element
// control vector is used to verify operations' correctness
// the check is done by getting all BS elements into a vector and comparing sorted data
TEST(methods, random)
{
	const int iterations = 1000;	   // How many times to attempt insert()/erase()
	const double delete_prob = 0.1;	   // Probability for erase()

	BucketStorage< S > bs(20);	  // Here you can call an alternative constructor
	std::vector< S > v;

	auto check = [&]()
	{
#if RANDOM_TEST_LOG
		print(bs);
#endif
		EXPECT_EQ(bs.size(), v.size());
		expect_same_elements(bs, v);
	};
	auto erase = [&](BucketStorage< S >::const_iterator bsit, std::vector< S >::const_iterator vit)
	{
		return std::make_pair(bs.erase(bsit), v.erase(vit));
	};

	for (int i = 0; i < iterations; i++)
	{
		double r = randdouble();
		if (r <= delete_prob)
		{
			if (v.empty())
				continue;
			int index = randint(0, int(v.size() - 1));
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
			int to_insert = Id::get_id();
			insert(bs, v, to_insert);
#if RANDOM_TEST_LOG
			std::cout << "inserting: " << to_insert << '\n';
#endif
			check();
		}
	}
}
TEST(methods, swap)
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

TEST(methods, empty)
{
	BucketStorage< S > ss(3);
	EXPECT_TRUE(ss.empty());
	ss.insert(S(1));
	EXPECT_FALSE(ss.empty());
	ss.insert(S(2));
	EXPECT_FALSE(ss.empty());
	for (auto it = ss.begin(); it != ss.end();)
	{
		it = ss.erase(it);
	}
	EXPECT_TRUE(ss.empty());
}

auto random_bs_v()
{
	BucketStorage< S > bs;
	std::vector< S > v;
	for (int i = 0; i < 200; ++i)
	{
		insert(bs, v, Id::get_id());
	}
	for (size_t i = 0; i < v.size(); i++)
	{
		if (randdouble() < 0.25)
		{
			bs.erase(std::find(bs.begin(), bs.end(), v[i]));
			v.erase(v.begin() + i);
		}
	}
	return std::make_pair(std::move(bs), std::move(v));
}

TEST(methods, shrink_to_fit)
{
	auto [bs, v] = random_bs_v();
	expect_same_elements(bs, v);
	// std::cout << "before: " << bs.capacity() << '\n';
	bs.shrink_to_fit();
	// std::cout << "after: " << bs.capacity() << '\n';
	expect_same_elements(bs, v);
}
TEST(methods, clear)
{
	auto [bs, v] = random_bs_v();
	expect_same_elements(bs, v);
	bs.clear();
	v.clear();
	EXPECT_EQ(bs.size(), 0);
	EXPECT_EQ(bs.capacity(), 0) << "By definition, empty bucket storage should deallocate all blocks";
	expect_same_elements(bs, v);
}

TEST(methods, get_to_distance)
{
	BucketStorage< S > bs(10);
	for (int i = 0; i < 20; ++i)
	{
		bs.insert(S(Id::get_id()));
	}
	auto it = bs.begin();
	size_t dist = 11;
	for (size_t i = 0; i < dist; ++i)
	{
		++it;
	}
	EXPECT_EQ(it, bs.get_to_distance(bs.begin(), dist));
}

TEST(methods, iterator_operators)
{
	auto expect_eq = [](BucketStorage< S >::iterator val1, BucketStorage< S >::iterator val2, const char *message)
	{
		EXPECT_TRUE(val1 == val2 && !(val1 != val2)) << message;
	};
	BucketStorage< S > bs(2);
	for (int i = 0; i < 5; ++i)
	{
		S value = S(Id::get_id());
		EXPECT_EQ(value, *bs.insert(value)) << "Unary * operator";
		EXPECT_EQ(value.x, bs.insert(value)->x) << "-> operator";
	}
	auto it = bs.begin();
	expect_eq(it++, bs.begin(), "postfix increment operator");
	EXPECT_TRUE(it >= bs.begin()) << ">= operator";
	EXPECT_TRUE(it > bs.begin()) << "> operator";
	expect_eq(++it, ++(++bs.begin()), "prefix increment operator");
	EXPECT_TRUE(it > ++bs.begin()) << "> operator";

	auto it2 = it;
	expect_eq(it2--, it, "postfix decrement operator");
	expect_eq(--it2, bs.begin(), "prefix decrement operator");

	auto it3 = bs.begin();
	EXPECT_TRUE(++bs.begin() >= bs.begin()) << ">= operator";
	EXPECT_FALSE(it3++ > bs.begin()) << "> operator";

	for (int i = 1; i <= 20; ++i)
	{
		bs.insert(S(Id::get_id()));
	}

	it = bs.begin();
	for (int i = 0; it != bs.end(); ++i)
	{
		if (i == 2 || i == 6 || i == 13 || i == 18)
		{
			it = bs.erase(it);
		}
		else
		{
			++it;
		}
	}
	it = bs.begin();
	for (int i = 0; i < 15; ++i)
	{
		auto prev = it++;
		auto next = it;
		++next;
		EXPECT_TRUE(it > prev);
		EXPECT_TRUE(prev < it);
		EXPECT_TRUE(next > prev);
		EXPECT_TRUE(prev < next);
		EXPECT_TRUE(next > it);
		EXPECT_TRUE(it < next);

		EXPECT_TRUE(it > bs.begin());
		EXPECT_TRUE(bs.begin() < it);
		EXPECT_TRUE(bs.end() > it);
		EXPECT_TRUE(it < bs.end());
		EXPECT_TRUE(--bs.end() > it);
		EXPECT_TRUE(it < --bs.end());
	}
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

TEST(typing, cbegin)
{
	const BucketStorage< S > const_bs;
	EXPECT_TRUE((std::same_as< decltype(const_bs.begin()), BucketStorage< S >::const_iterator >))
		<< "begin() should have a "
		   "const overload that "
		   "returns const_iterator";
	EXPECT_TRUE((std::same_as< decltype(const_bs.end()), BucketStorage< S >::const_iterator >))
		<< "end() should have a "
		   "const overload that "
		   "returns const_iterator";
	BucketStorage< S > bs;
	EXPECT_TRUE((std::same_as< decltype(bs.begin()), BucketStorage< S >::iterator >));
	EXPECT_TRUE((std::same_as< decltype(bs.end()), BucketStorage< S >::iterator >));

	EXPECT_TRUE((std::same_as< decltype(bs.cbegin()), BucketStorage< S >::const_iterator >));
	EXPECT_TRUE((std::same_as< decltype(bs.cend()), BucketStorage< S >::const_iterator >));

	BucketStorage< S >::const_iterator const_it = bs.begin();	 // implicit conversion
	(void)const_it;
}

TEST(typing, const_bs)
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
	{ a.begin() } -> std::same_as< typename ContainerType::iterator >;
	{ a.end() } -> std::same_as< typename ContainerType::iterator >;
	{ b.begin() } -> std::same_as< typename ContainerType::const_iterator >;
	{ b.end() } -> std::same_as< typename ContainerType::const_iterator >;
	{ a.cbegin() } -> std::same_as< typename ContainerType::const_iterator >;
	{ a.cend() } -> std::same_as< typename ContainerType::const_iterator >;
	{ a.size() } -> std::same_as< typename ContainerType::size_type >;
	/*
	{
		// maybe we don't really need this
		// unless you find a sensible way to calculate this
		a.max_size()
	} -> std::same_as< typename ContainerType::size_type >;
	*/
	{ a.empty() } -> std::same_as< bool >;
};
template< Container C >
void container_f(C c)
{
	(void)c;
}

TEST(typing, concepts)
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

int iterations = 10000;
const double delete_prob = 0.2;

// Compile this test with -fsanitize=address
TEST(methods, memory_leaks)
{
	BucketStorage< M > bs(20);

	for (int i = 0; i < 1000; i++)
	{
		double r = randdouble();
		if (r <= delete_prob && !bs.empty())
		{
			int pos = randint(0, bs.size() - 1);
			auto it = bs.begin();
			for (int j = 0; j < pos; ++j)
			{	 // iteration
				++it;
			}
			bs.erase(it);	 // erase
		}
		else
		{
			bs.insert(M(Id::get_id()));	   // insert
		}
	}
}

// A relative benchmark that can be used to optimize the data structure.
// Includes inserting, erasing and iterating (before every erase).
// NOTE: pass in any integer as commandline arguments to change the iterations value
TEST(benchmark, insert_erase_iter)
{
	std::cout << "Benchmark: " << iterations << " iterations\n";
	BucketStorage< S > bs;

	for (int i = 0; i < iterations; i++)
	{
		double r = randdouble();
		if (r <= delete_prob && !bs.empty())
		{
			int pos = randint(0, bs.size() - 1);
			auto it = bs.begin();
			for (int j = 0; j < pos; ++j)
			{	 // iteration
				++it;
			}
			bs.erase(it);	 // erase
		}
		else
		{
			bs.insert(S(Id::get_id()));	   // insert
		}
	}
}

// This is the control benchmark of the same operations as the above test
// performed with a vector to compare gains in speed.
TEST(benchmark, insert_erase_iter_vector)
{
	std::vector< S > v;

	for (int i = 0; i < iterations; i++)
	{
		double r = randdouble();
		if (r <= delete_prob && !v.empty())
		{
			int pos = randint(0, v.size() - 1);
			auto it = v.begin();
			for (int j = 0; j < pos; ++j)
			{	 // iteration
				++it;
			}
			v.erase(it);	// erase
		}
		else
		{
			v.push_back(S(Id::get_id()));	 // insert
		}
	}
}

class TraceHandler : public testing::EmptyTestEventListener
{
	// Called after a test ends.
	void OnTestEnd(const testing::TestInfo &test_info) override
	{
		(void)test_info;
		S::actions.clear();
		Id::reset();
	}
};

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	if (argc > 1)
	{
		iterations = std::atoi(argv[1]);
	}

	testing::TestEventListeners &listeners = testing::UnitTest::GetInstance()->listeners();
	listeners.Append(new TraceHandler);

	return RUN_ALL_TESTS();
}

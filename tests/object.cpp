#include "lox/object.hpp"

#define BOOST_TEST_MODULE ObjectTests
#include <boost/test/included/unit_test.hpp>

using namespace lox;

BOOST_AUTO_TEST_CASE( values )
{
	object str{"asdf"};
	object num{3.14159};
	object f{false};
	object nil{nullptr};

	BOOST_TEST(static_cast<std::string>(str) == std::string{"asdf"});
	BOOST_TEST(static_cast<double>(num) == 3.14159);
	BOOST_TEST(static_cast<bool>(f) == false);
	BOOST_TEST(static_cast<nullptr_t>(nil) == nullptr);
}

BOOST_AUTO_TEST_CASE(unary_ops)
{
	object f{false};
	object num{1};

	BOOST_TEST(static_cast<bool>(!f));
	BOOST_TEST(static_cast<double>(-num) == -1.0);
}

BOOST_AUTO_TEST_CASE(comparisons_ops)
{
	object num1{1};
	object num2{2};

	BOOST_TEST(num1 < num2);
	BOOST_TEST(num1 <= num2);
	BOOST_TEST(num2 <= num2);
	BOOST_TEST(num2 == num2);
	BOOST_TEST(!(num1 == num2));
	BOOST_TEST(num1 != num2);
	BOOST_TEST(num2 > num1);
	BOOST_TEST(num2 >= num1);
}

BOOST_AUTO_TEST_CASE(arithmetic)
{
	object num1{30};
	object num2{5};

	BOOST_TEST(static_cast<double>(num1 + num2) == 35);
	BOOST_TEST(static_cast<double>(num1 - num2) == 25);
	BOOST_TEST(static_cast<double>(num1 * num2) == 150);
	BOOST_TEST(static_cast<double>(num1 / num2) == 6);

	object f{false};
	object s{"asdf"};
	object nil{};

	BOOST_CHECK_THROW(num1 + f, type_error);
	BOOST_CHECK_THROW(num1 - f, type_error);
	BOOST_CHECK_THROW(num1 * f, type_error);
	BOOST_CHECK_THROW(num1 / f, type_error);

	BOOST_CHECK_THROW(num1 + s, type_error);
	BOOST_CHECK_THROW(num1 - s, type_error);
	BOOST_CHECK_THROW(num1 * s, type_error);
	BOOST_CHECK_THROW(num1 / s, type_error);

	BOOST_CHECK_THROW(num1 + nil, type_error);
	BOOST_CHECK_THROW(num1 - nil, type_error);
	BOOST_CHECK_THROW(num1 * nil, type_error);
	BOOST_CHECK_THROW(num1 / nil, type_error);
}

BOOST_AUTO_TEST_CASE(str_conv)
{
	object str{"asdf"};
	object num{123.456};
	object t{true};
	object f{false};
	object nil{};
	
	BOOST_TEST(str.str() == "asdf");
	BOOST_TEST(num.str() == "123.456000");
	BOOST_TEST(t.str() == "true");
	BOOST_TEST(f.str() == "false");
}


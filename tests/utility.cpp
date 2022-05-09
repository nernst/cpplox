#include <array>
#include <string>
#include <utility>
#include <boost/test/unit_test.hpp>

#include "lox/utility.hpp"

using namespace lox;
using namespace std::literals::string_literals;

BOOST_AUTO_TEST_CASE(utility_trim)
{
	std::array<std::pair<std::string, std::string>, 6> tests{{
		{ "  asdf ", "asdf" },
		{ "\tasdf\n\t ", "asdf" },
		{ " \rasdf ", "asdf"},
		{ "     ", "" },
		{ "asdf", "asdf"},
		{ "asdf\n", "asdf"},
	}};

	for(auto&& p : tests)
		BOOST_REQUIRE_EQUAL(trim(p.first), p.second);
}


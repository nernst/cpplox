#include "lox/scanner.hpp"
#include <string>
#include <tuple>

#include <boost/test/unit_test.hpp>
#include <fmt/format.h>

using namespace std::literals::string_literals;
using namespace  lox;

inline std::tuple<bool, std::vector<Token>> run_scanner(std::string const& source)
{
	std::vector<Token> tokens;
	bool had_error{false};

	Scanner scanner{source};
	while (true)
	{
		auto token = scanner.scan();
		if (token.type == Token::TOKEN_EOF)
		{
			tokens.push_back(std::move(token));
			break;
		}

		if (token.type == Token::ERROR)
		{
			fmt::print(stderr, "test: error at line {}: {}", token.line, token.message);
			had_error = true;
		}
		tokens.push_back(std::move(token));
	}

	return std::make_tuple(had_error, std::move(tokens));
}

BOOST_AUTO_TEST_CASE(simple_tokens)
{
	auto test = R"test(
(
)
{
}
,
.
-
=
;
*
!
=
<
>
/
// comment
<=
>=
==
!=
)test"s;

	auto [had_error, tokens] = run_scanner(test);

	BOOST_TEST(!had_error);
}

BOOST_AUTO_TEST_CASE(keywords)
{
	auto test = R"test(
and
class
else
flase
for
fun
if
nil
or
print
return
super
this
true
var
while
)test"s;

	auto [had_error, tokens] = run_scanner(test);

	BOOST_TEST(!had_error);
}

BOOST_AUTO_TEST_CASE(literals)
{
	auto test = R"test(
1234.00
"asdf"
"this
is a multiline
string"
true
false
nil
)test"s;

	auto [had_error, tokens] = run_scanner(test);

	BOOST_TEST(!had_error);
}

BOOST_AUTO_TEST_CASE(expression)
{
	auto test = "!(5 - 4 > 3 * 2 == !nil)"s;
	auto [had_error, tokens] = run_scanner(test);

	BOOST_TEST(!had_error);
	BOOST_TEST(tokens.size() == 14);
	BOOST_TEST(tokens[0].type == Token::BANG);
	BOOST_TEST(tokens[1].type == Token::LEFT_PAREN);
	BOOST_TEST(tokens[2].type == Token::NUMBER);
	BOOST_TEST(tokens[3].type == Token::MINUS);
	BOOST_TEST(tokens[4].type == Token::NUMBER);
	BOOST_TEST(tokens[5].type == Token::GREATER);
	BOOST_TEST(tokens[6].type == Token::NUMBER);
	BOOST_TEST(tokens[7].type == Token::STAR);
	BOOST_TEST(tokens[8].type == Token::NUMBER);
	BOOST_TEST(tokens[9].type == Token::EQUAL_EQUAL);
	BOOST_TEST(tokens[10].type == Token::BANG);
	BOOST_TEST(tokens[11].type == Token::NIL);
	BOOST_TEST(tokens[12].type == Token::RIGHT_PAREN);
	BOOST_TEST(tokens[13].type == Token::TOKEN_EOF);
}

#include "lox/scanner.hpp"
#include <string>
#include <tuple>

#include <boost/test/unit_test.hpp>

using namespace std::literals::string_literals;
using namespace  lox;

inline std::tuple<bool, std::vector<token>> run_scanner(source& s)
{
	std::vector<token> tokens;
	bool had_error{false};

	auto token_handler = [&tokens](token&& tok)
	{
		tokens.push_back(std::move(tok));
	};

	auto error_handler = [&had_error](source const& s, std::size_t offset, std::string_view where, std::string_view message)
	{
		ignore_unused(s, offset, where, message);

		had_error = true;
	};

	using scanner_t = scanner<decltype(token_handler), decltype(error_handler)>;

	scanner_t scan{s, std::move(token_handler), std::move(error_handler)};
	scan.scan();

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

	string_source s{"simple_tokens", test};

	auto [had_error, tokens] = run_scanner(s);

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

	string_source s{"keywords", test};

	auto [had_error, tokens] = run_scanner(s);

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

	string_source s{"literals", test};
	auto [had_error, tokens] = run_scanner(s);

	BOOST_TEST(!had_error);
}


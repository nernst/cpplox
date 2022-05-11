#include "lox/parser.hpp"
#include "lox/scanner.hpp"
#include <string>
#include <sstream>
#include <tuple>

#include <boost/test/unit_test.hpp>

using namespace std::literals::string_literals;
using namespace  lox;

inline std::tuple<bool, parser::statement_vec, std::string> run_parser(source& s)
{
	std::vector<token> tokens;

	auto token_handler = [&tokens](token&& tok)
	{
		tokens.push_back(std::move(tok));
	};

	auto error_handler = [](source const& s, std::size_t offset, std::string_view where, std::string_view message)
	{
		ignore_unused(s, offset, where, message);
	};

	using scanner_t = scanner<decltype(token_handler), decltype(error_handler)>;

	scanner_t scan{s, std::move(token_handler), std::move(error_handler)};
	scan.scan();

	std::stringstream error;

	parser p{std::move(tokens), &error};

	auto [had_error, statements] = p.parse();

	return std::make_tuple(had_error, std::move(statements), error.str()); 
}

BOOST_AUTO_TEST_CASE(true_synax_error)
{
	auto test = R"test(true)test"s;

	string_source s{"true_syntax_error", test};

	auto [had_error, statements, error] = run_parser(s);
	ignore_unused(statements, error);
	BOOST_TEST(had_error);
}


#include "lox/parser.hpp"
#include "lox/scanner.hpp"
#include <string>
#include <tuple>

#include <boost/test/unit_test.hpp>

using namespace std::literals::string_literals;
using namespace  lox;

inline std::tuple<bool, parser::statement_vec> run_parser(source& s)
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

	parser p{std::move(tokens)};

	return p.parse();
}

BOOST_AUTO_TEST_CASE(true_synax_error)
{
	auto test = R"test(true)test"s;

	string_source s{"true_syntax_error", test};

	std::cerr << "true_syntax_error: expect syntax error message:" << std::endl;
	auto [had_error, statements] = run_parser(s);
	ignore_unused(statements);
	std::cerr << "true_syntax_error: end syntax error message" << std::endl;
	BOOST_TEST(had_error);
}


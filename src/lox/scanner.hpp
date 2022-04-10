#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include "token.hpp"
#include "utility.hpp"

namespace lox {

	struct scanner_error_handler
	{
		void operator()(
			std::string_view const& source_path, 
			std::size_t offset, 
			std::size_t line_number,
			std::string const& where,
			std::string const& message
			)
		{
			ignore_unused(source_path, offset);
			std::cerr << "[line " << line_number << "] Error" << where << ": " << message << '\n';
		}
	};


	template<typename ErrorHandler = scanner_error_handler>
	class scanner
	{
	public:

		explicit scanner(
			std::string_view source_path,
			std::string_view source,
			ErrorHandler&& error_handler = ErrorHandler()
		)
		: source_path_{source_path}
		, source_{source}
		, error_handler_{std::forward<ErrorHandler>(error_handler)}
		, current_{0}
		, end_{source.size()}
		, had_error_{false}
		{ }

		template<typename TokenSink>
		void scan(TokenSink&& sink);

		std::size_t line_number(std::size_t offset) const {
			assert(offset <= end_);

			auto i = std::lower_bound(std::cbegin(lines_), std::cend(lines_), offset);
			return std::distance(i, std::cbegin(lines_));
		}

		bool had_error() const { return had_error_; }

	private:
		std::string_view source_path_;
		std::string_view source_;
		ErrorHandler error_handler_;
		std::vector<std::size_t> lines_;
		std::size_t current_;
		std::size_t end_;
		bool had_error_;
	};

	template<typename ErrorHandler>
	template<typename TokenSink>
	inline void scanner<ErrorHandler>::scan(TokenSink&& sink)
	{
		ignore_unused(sink);
	}


} // namespace lox


#pragma once

#include <exception>
#include <source_location>
#include <string>
#include <memory>

#include <fmt/format.h>

#include "token_type.hpp"

#define LOX_THROW(type, msg, ...) throw (type){msg __VA_OPT__(,) __VA_ARGS__}
#define LOX_NOT_IMPLEMENTED() LOX_THROW(not_implemented, "not implemented")

namespace lox
{
	class error : public std::exception
	{
	public:
		virtual ~error() noexcept {}

		error() = default;
		error(error const&) = default;
		error(error&&) = default;

		error& operator=(error const&) = default;
		error& operator=(error&&) = default;

		template<class T>
		explicit error(
			T&& message,
			const std::source_location location = std::source_location::current()
		)
		: message_{std::forward<T>(message)}
		, location_{location}
		{ }

		virtual std::unique_ptr<error> clone() const
		{ return std::make_unique<error>(*this); }

		const char* what() const noexcept { return message_.c_str(); }
		const char* file() const noexcept { return location_.file_name(); }
		std::size_t line_no() const noexcept { return location_.line(); }

		std::string str() const
		{
			return fmt::format("{} @ [{}:{}]", what(), file(), line_no());
		}

	private:
		std::string message_;
		std::source_location location_;
	};

	class not_implemented : public error
	{
	public:
		using error::error;

		std::unique_ptr<error> clone() const override
		{ return std::make_unique<not_implemented>(*this); }
	};


	class type_error : public error
	{
	public:
		using error::error;

		std::unique_ptr<error> clone() const override
		{ return std::make_unique<type_error>(*this); }
	};

	class programming_error : public error
	{
	public:
		using error::error;

		std::unique_ptr<error> clone() const override
		{ return std::make_unique<programming_error>(*this); }
	};

	class runtime_error : public error
	{
	public:
		using error::error;

		std::unique_ptr<error> clone() const override
		{ return std::make_unique<runtime_error>(*this); }
	};


	class parse_error : public error
	{
	public:

		using error::error;

		template<typename MsgType>
		explicit parse_error(
			token_type type, 
			MsgType&& message,
			const std::source_location location = std::source_location::current()
		)
		: error{std::forward<MsgType>(message), location}
		, type_{type}
		{ }

		parse_error(parse_error const&) = default;
		parse_error(parse_error&&) = default;

		parse_error& operator=(parse_error const&) = default;
		parse_error& operator=(parse_error&&) = default;

		token_type type() const { return type_; }

		std::unique_ptr<error> clone() const override
		{ return std::make_unique<parse_error>(*this); }

	private:
		token_type type_;
		std::string_view message_;
	};

	template<typename ExType, typename... Args>
	[[noreturn]]
	void throwex(Args&&... args, const std::source_location location = std::source_location::current())
	{
		throw ExType{std::forward<Args>(args)..., location};
	}

} // namespace lox


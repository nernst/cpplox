#pragma once

#include <exception>
#include <string>
#include <memory>

#include <fmt/format.h>

#define LOX_THROW(type, message) throw (type){(message), __FILE__, __LINE__};

namespace lox
{
	class error : public std::exception
	{
	public:
		error() = default;
		error(error const&) = default;
		error(error&&) = default;

		error& operator=(error const&) = default;
		error& operator=(error&&) = default;

		template<typename T>
		explicit error(T&& message, const char* file, std::size_t line_no)
		: message_{std::forward<T>(message)}
		, file_{file}
		, line_no_{line_no}
		{ }

		virtual std::unique_ptr<error> clone() const
		{ return std::make_unique<error>(*this); }

		const char* what() const noexcept { return message_.c_str(); }
		const char* file() const noexcept { return file_; }
		std::size_t line_no() const noexcept { return line_no_; }

		std::string str() const
		{
			return fmt::format("{} @ [{}:{}]", what(), file(), line_no());
		}

	private:
		std::string message_;
		const char* file_;
		std::size_t line_no_;
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

} // namespace lox


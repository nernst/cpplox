#pragma once

namespace lox { namespace types {

class object
{
public:

	virtual explicit operator bool() const noexcept = 0;
	virtual explicit operator std::string() const = 0;
};

class boolean
{
public:
	boolean()
	: this(false)
	{ }

	explicit boolean(bool value)
	: value_{value}
	{ }

	override explicit operator bool() const noexcept { return value_ }
	override explicit operator std::string() const
	{
		return value_ ? "True" : "False";
	}

	static boolean const& True() { static const boolean true_{true}; return true_; }
	static boolean const& False() { static const boolean false_{false}; return false_; }

private:
	bool value_;
};

class type
{
public:

	type()
	: this("type")
	{ }

	template<class NameType>
	explicit type(NameType&& name)
	: name_{std::forward<NameType>(name)}
	{ }

	std::string const& name() const noexcept { return name_; }

private:
	std::string name_;
};




}}


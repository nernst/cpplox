#pragma once

#include <algorithm>
#include <cassert>
#include <iterator>
#include <string>
#include <string_view>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>


namespace lox {

	// TODO: Handle UTF-8 byte-order-mark
	class source
	{
	public:
		virtual ~source() { }

		source()
		: lines_(1, 0)
		{ }

		virtual std::string const& name() const = 0;

		virtual std::string_view view() const = 0;
		virtual std::size_t size() const = 0;
		virtual const char* cbegin() const = 0;
		virtual const char* cend() const = 0;

		char operator[](std::size_t offset) const
		{
			assert(offset < size());
			return cbegin()[offset];
		}

		std::string_view substr(std::size_t offset, std::size_t len) const
		{
			assert(offset < size());
			assert(offset + len < size());

			auto start = cbegin() + offset;

			return std::string_view{start, len};
		}

		void add_line(std::size_t offset)
		{
			assert(offset <= size());
			lines_.push_back(offset);
		}

		// (line_number, line_offset, line)
		std::tuple<std::size_t, std::size_t, std::string_view> get_line(std::size_t offset) const
		{
			assert(offset <= size());
			auto i = std::lower_bound(std::cbegin(lines_), std::cend(lines_), offset);
			if (*i >= offset)
				--i;
			
			assert(i != std::cend(lines_));
			auto line_start{*i};
			auto end = std::find(cbegin() + line_start, cend(), '\n');

			return std::make_tuple(
				std::distance(std::cbegin(lines_), i),
				offset - 1 - line_start,
				std::string_view{cbegin() + line_start, static_cast<std::size_t>(std::distance(cbegin() + line_start, end))}
			);
		}

		std::size_t line_no(std::size_t offset) const
		{
			assert(offset < size());
			auto i = std::lower_bound(std::cbegin(lines_), std::cend(lines_), offset);
			return std::distance(std::cbegin(lines_), i);
		}

	private:
		std::vector<std::size_t> lines_;
	};


	class file_source : public source
	{
	public:

		template<class FilePath>
		explicit file_source(FilePath&& file_path)
		: file_path_{std::forward<FilePath>(file_path)}
		, file_{file_path_.c_str(), boost::interprocess::read_only}
		, region_{file_, boost::interprocess::read_only}
		{ }

		file_source(file_source&& other) = default;
		file_source& operator=(file_source&&) = default;
		
		~file_source() = default;

		file_source& operator=(file_source const&) = delete;
		file_source(file_source const&) = delete;

		std::string const& name() const override { return file_path_; }


		std::string_view view() const override {
			return std::string_view{cbegin(), size()};
		}

		std::size_t size() const override {
			return region_.get_size();
		}

		const char* cbegin() const override {
			return static_cast<const char*>(region_.get_address());
		}

		const char* cend() const override {
			return cbegin() + size();
		}

	private:
		std::string file_path_;
		boost::interprocess::file_mapping file_;
		boost::interprocess::mapped_region region_;	
	};

	class string_source : public source
	{
	public:
		template<class NameType, class Contents>
		explicit string_source(NameType&& name, Contents&& contents)
		: name_{std::forward<NameType>(name)}
		, contents_{std::forward<Contents>(contents)}
		{ }

		std::string const& name() const override { return name_; }

		std::string_view view() const override { return std::string_view{contents_}; }
		std::size_t size() const override { return contents_.size(); }

		const char* cbegin() const override { return contents_.data(); }
		const char* cend() const override { return contents_.data() + contents_.size(); }

	private:
		std::string name_;
		std::string contents_;
	};

	class location
	{
	public:
		location(source const& where, std::size_t offset)
		: where_{&where}
		, offset_{offset}
		{ }

		location(location const&) = default;
		location(location&&) = default;

		location& operator=(location const&) = default;
		location& operator=(location&&) = default;

		source const& where() const { return *where_; }
		std::size_t offset() const { return offset_; }
	
		// [line_no, line_offset, line]
		std::tuple<std::size_t, std::size_t, std::string_view> get_line() const
		{ return where().get_line(offset()); }

		std::size_t line() const { return where().line_no(offset()); }

	private:
		source const* where_;
		std::size_t offset_;
	};


} // namespace lox


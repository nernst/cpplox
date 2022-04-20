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

} // namespace lox


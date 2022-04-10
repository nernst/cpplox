#include <string_view>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>


namespace lox {

	class source_file
	{
	public:

		explicit source_file(std::string_view file_path)
		: file_path_{file_path}
		, file_{file_path.data(), boost::interprocess::read_only}
		, region_{file_, boost::interprocess::read_only}
		{ }

		source_file(source_file&& other) = default;
		source_file& operator=(source_file&&) = default;
		
		~source_file() = default;

		source_file& operator=(source_file const&) = delete;
		source_file(source_file const&) = delete;


		std::string_view view() const {
			return std::string_view{cbegin(), size()};
		}

		std::size_t size() const {
			return region_.get_size();
		}

		const char* cbegin() const {
			return static_cast<const char*>(region_.get_address());
		}

		const char* cend() const {
			return cbegin() + size();
		}

	private:
		std::string_view file_path_;
		boost::interprocess::file_mapping file_;
		boost::interprocess::mapped_region region_;	
	};

} // namespace lox


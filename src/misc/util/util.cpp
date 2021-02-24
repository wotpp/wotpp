#include <string>
#include <filesystem>
#include <fstream>
#include <array>
#include <stdexcept>

#include <cstdint>
#include <cstdio>


namespace wpp {
	// FNV-1a hash.
	// Calculate a hash of a range of bytes.
	uint64_t hash_bytes(const char* begin, const char* const end) {
		uint64_t offset_basis = 0;
		uint64_t prime = 0;

		offset_basis = 14'695'981'039'346'656'037u;
		prime = 1'099'511'628'211u;

		uint64_t hash = offset_basis;

		while (begin != end) {
			hash = (hash ^ static_cast<uint64_t>(*begin)) * prime;
			begin++;
		}

		return hash;
	}


	// Execute a shell command, capture its standard output and return it
	// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
	std::string exec(const std::string& cmd, int& rc) {
		#if !defined(WPP_DISABLE_RUN)
			std::array<char, 128> buffer;
			std::string result;

			FILE* pipe = popen(cmd.c_str(), "r");

			if (not pipe) {
				throw std::runtime_error("popen() failed!");
			}

			while (not std::feof(pipe)) {
				if (std::fgets(buffer.data(), buffer.size(), pipe) != nullptr)
					result += buffer.data();
			}

			rc = pclose(pipe);

			return result;
		#else
			(void)cmd;  // Hide "unused" warnings.
			(void)rc;

			return "";
		#endif
	}


	// Read a file into a string relatively quickly.
	std::string read_file(const std::string& fname) {
		auto filesize = std::filesystem::file_size(fname);
		std::ifstream is(fname, std::ios::binary);

		auto str = std::string(filesize + 1, '\0');
		is.read(str.data(), static_cast<std::streamsize>(filesize));

		return str;
	}
}


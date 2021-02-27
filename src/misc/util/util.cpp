#include <string>
#include <filesystem>
#include <fstream>
#include <array>
#include <stdexcept>

#include <cstdint>
#include <cstdio>


#if !defined(WPP_DISABLE_RUN)
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <sys/mman.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <dlfcn.h>
#endif


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


	std::string exec(const std::string& cmd, const std::string& data, int& rc) {
		#if !defined(WPP_DISABLE_RUN)
			int stdin_pipe[2];
			int stdout_pipe[2];

			if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
				rc = 1;
				return "";
			}

			pid_t child = fork();

			std::string out;

			if (not child) {
				dup2(stdin_pipe[0], STDIN_FILENO);
				dup2(stdout_pipe[1], STDOUT_FILENO);
				dup2(stdout_pipe[1], STDERR_FILENO);

				close(stdin_pipe[0]);
				close(stdin_pipe[1]);
				close(stdout_pipe[0]);
				close(stdout_pipe[1]);

				execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
			}

			else {
				close(stdout_pipe[1]);
				close(stdin_pipe[0]);

				write(stdin_pipe[1], data.c_str(), data.size());
				close(stdin_pipe[1]);

				int wstatus;
				wait(&wstatus);

				rc = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;

				ssize_t n;
				char buf[4096];

				while ((n = read(stdout_pipe[0], buf, 4096)) > 0) {
					out += std::string_view{buf, size_t(n)};
				}

				close(stdout_pipe[0]);
			}

			return out;
		#else
			(void)cmd;
			(void)data;
			(void)rc;
			return "";
		#endif
	}

	// Read a file into a string relatively quickly.
	std::string read_file(std::string_view fname) {
		auto filesize = std::filesystem::file_size(fname);
		std::ifstream is(fname.data());

		auto str = std::string(filesize + 1, '\0');
		is.read(str.data(), static_cast<std::streamsize>(filesize));

		return str;
	}


	void write_file(std::string_view fname, const std::string& contents) {
		auto file = std::ofstream(fname.data());
		file << contents;
		file.close();
	}
}


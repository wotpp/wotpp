#include <string>
#include <array>

#include <cstdint>
#include <cstdio>

#if !defined(WPP_DISABLE_RUN)
	#include <sys/wait.h>
	#include <unistd.h>
#endif

namespace wpp {
	// Execute a shell command, capture its standard output and return it
	// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
	#if !defined(WPP_DISABLE_RUN)
		std::string exec(const std::string& cmd, int& rc) {
			std::array<char, 128> buffer;
			std::string result;

			FILE* pipe = popen(cmd.c_str(), "r");

			if (not pipe) {
				rc = 1;
				return "";
			}

			while (not std::feof(pipe)) {
				if (std::fgets(buffer.data(), buffer.size(), pipe) != nullptr)
					result += buffer.data();
			}

			rc = pclose(pipe);

			return result;
		}

	#else
		std::string exec(const std::string&, int&) {
			return "";
		}
	#endif


	#if !defined(WPP_DISABLE_RUN)
		std::string exec(const std::string& cmd, const std::string& data, int& rc) {
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

				(void)write(stdin_pipe[1], data.c_str(), data.size());
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
		}

	#else
		std::string exec(const std::string&, const std::string&, int&) {
			return "";
		}
	#endif
}


/*
 * There is a lot of quick hacked together code here;
 * mostly to just get things working in my local environment.
 * Debatable use case.
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "GVizUtils.h"

#include <iostream>

/*
 * Use dot(1) and convert .dot to a .ps file
 *
 */
bool
GVizUtils::dot2ps(std::string dotFile, std::string psFile)
{
	int pid = fork();
	if (pid == 0) {
		int sin = open("/dev/null", O_RDONLY);
		int serr = open("/dev/null", O_WRONLY);
		int sout = open("/dev/null", O_WRONLY);

		dup2(sin, STDIN_FILENO);
		dup2(serr, STDERR_FILENO);
		dup2(sout, STDOUT_FILENO);

		close(sin);
		close(serr);
		close(sout);
		execl("/usr/bin/dot", "dot", "-Tps", dotFile.c_str(), "-o",
		  psFile.c_str(), NULL);
	} else {
		GVizUtils::waitForIt(pid); // XXX return value
	}
	return true;
}

/*
 * Launches gv(1) with the given .ps file.
 *
 */
int
GVizUtils::launchGV(std::string psFile)
{
	int pid = fork();
	if (pid == 0) {
		int sin = open("/dev/null", O_RDONLY);
		int serr = open("/dev/null", O_WRONLY);
		int sout = open("/dev/null", O_WRONLY);

		dup2(sin, STDIN_FILENO);
		dup2(serr, STDERR_FILENO);
		dup2(sout, STDOUT_FILENO);

		close(sin);
		close(serr);
		close(sout);

		execl("/usr/bin/gv", "gv", psFile.c_str(), NULL);
    }
	return pid;
}

/*
 * Wait wrapper.
 *
 */
bool
GVizUtils::waitForIt(int pid)
{
	int ws = 0;
	int w = -1;

	do {
		w = waitpid(pid, &ws, WUNTRACED | WCONTINUED);
		if (w == -1) {
			perror("waitpid");
			return false;
		}
	} while (!WIFEXITED(ws) && !WIFSIGNALED(ws));
	return true;
}

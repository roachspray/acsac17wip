#ifndef	__GVIZUTILS_H
#define	__GVIZUTILS_H

#include <string>

class GVizUtils {

public:
	static bool	dot2ps(std::string dotFilePath, std::string psFilePath);

	static int launchGV(std::string psFile);

	static bool waitForIt(int pid);
};
#endif

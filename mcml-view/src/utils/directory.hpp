#pragma once

#include <string>

class DirUtil
{
public:
	// get the full path to the current executable
	static std::string getCurrentExecutablePath();

	// starts looking from the current directory upwards until it discovers a valid subdirectory described by assetDir
	// for example
	// getAssetPath("/home/someuser/projects/xxx/build/bin", "assets"); will return /home/someuser/projects/xxx/assets if this directory exists
	static std::string getAssetPath(std::string baseDir, const std::string& assetDir);
};
#if defined(_WIN32)
#   include <Windows.h>
#else
#   include <dlfcn.h>
#   include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32)
namespace
{
	// local function used to identify the current module on Windows
	void getAddr() {}
}
#endif

std::string DirUtil::getCurrentExecutablePath()
{
	std::string modulePath;
#if defined(_WIN32)
	HMODULE engine;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)getAddr,
		&engine);

	char path[_MAX_PATH + _MAX_FNAME + 1];
	GetModuleFileNameA(engine, path, _MAX_PATH + _MAX_FNAME);

	// normalize path
	char* p = path;
	while (*p)
	{
		if (*p == '\\')
		{
			*p = '/';
		}
		++p;
	}

	modulePath = path;

#else
	// retrieve the executable path and hope for the best
	char buff[2048];
	size_t len = readlink("/proc/self/exe", buff, sizeof(buff) - 1);
	if (len > 0)
	{
		buff[len] = 0;
		modulePath = buff;
	}

#endif

	return modulePath;
}

std::string DirUtil::getAssetPath(std::string baseDir, const std::string& assetDir)
{
	while (true)
	{
		auto slash = baseDir.rfind('/');
		if (slash == std::string::npos)
		{
			baseDir = assetDir;
			break;
		}

		baseDir.erase(slash + 1);

		baseDir += assetDir;

		struct stat info;
		if (stat(baseDir.c_str(), &info) == 0 && (info.st_mode & S_IFDIR))
		{
			break;
		}

		baseDir.erase(slash);
	}

	return baseDir;
}
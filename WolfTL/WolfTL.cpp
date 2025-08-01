﻿/*
 *  File: WolfTL.cpp
 *  Copyright (c) 2024 Sinflower
 *
 *  MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#include <Windows.h>
#include <tchar.h>

#include <filesystem>
#include <format>
#include <iostream>
#include <map>

#include "WolfRPG\WolfRPG.h"

namespace fs = std::filesystem;

static const std::string VERSION = "0.5.1";

/*
TODO:
 - Add an option to ignore the name sanity check in the data patching
 - Rewrite error messages to use std::format
*/

// From: https://stackoverflow.com/a/45588456
class MBuf : public std::stringbuf
{
public:
	int sync()
	{
		fputs(str().c_str(), stdout);
		str("");
		return 0;
	}
};

static MBuf g_buf;

void EnableUTF8Print()
{
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IONBF, 0);
	std::cout.rdbuf(&g_buf);
}

class WolfTL
{
	inline static const tString OUTPUT_DIR   = TEXT("dump/");
	inline static const tString MAP_OUTPUT   = OUTPUT_DIR + TEXT("mps/");
	inline static const tString DB_OUTPUT    = OUTPUT_DIR + TEXT("db/");
	inline static const tString COM_OUTPUT   = OUTPUT_DIR + TEXT("common/");
	inline static const tString PATCHED_DATA = TEXT("/patched/data/");

public:
	WolfTL(const tString& dataPath, const tString& outputPath, const bool& skipGD = false) :
		m_dataPath(dataPath),
		m_outputPath(outputPath),
		m_wolf(dataPath, skipGD),
		m_skipGD(skipGD)
	{
	}

	void ToJson() const
	{
		if (!m_wolf.Valid())
		{
			std::cerr << ERROR_TAG << "WolfRPG initialization failed" << std::endl;
			return;
		}

		maps2Json();
		databases2Json();
		commonEvents2Json();
		gameDat2Json();
	}

	void Patch(const bool& inplace = false)
	{
		// Skip backup if not patching in-place
		wolfRPGUtils::g_skipBackup = !inplace;

		if (!m_wolf.Valid())
		{
			std::cerr << ERROR_TAG << "WolfRPG initialization failed" << std::endl;
			return;
		}

		// Check if the patch folder exists
		if (!fs::exists(m_outputPath))
		{
			std::cerr << ERROR_TAG << "Patch folder does not exist" << std::endl;
			return;
		}

		patchMaps(m_outputPath);
		patchDatabases(m_outputPath);
		patchCommonEvents(m_outputPath);
		patchGameDat(m_outputPath);

		// Save the patched data
		m_wolf.Save2File((inplace ? m_dataPath : (m_outputPath + PATCHED_DATA)));
	}

private:
	void maps2Json() const
	{
		std::cout << "Writing Maps to JSON ... " << std::flush;

		const tString mapOutput = std::format(TEXT("{}/{}"), m_outputPath, MAP_OUTPUT);

		// Make sure the output folder exists
		fs::create_directories(mapOutput);

		for (const Map& map : m_wolf.GetMaps())
			map.ToJson(mapOutput);

		std::cout << "Done" << std::endl;
	}

	void databases2Json() const
	{
		std::cout << "Writing Databases to JSON ... " << std::flush;

		const tString dbOutput = std::format(TEXT("{}/{}"), m_outputPath, DB_OUTPUT);

		// Make sure the output folder exists
		fs::create_directories(dbOutput);

		for (const Database& db : m_wolf.GetDatabases())
			db.ToJson(dbOutput);

		std::cout << "Done" << std::endl;
	}

	void commonEvents2Json() const
	{
		std::cout << "Writing CommonEvents to JSON ... " << std::flush;

		const tString comOutput = std::format(TEXT("{}/{}"), m_outputPath, COM_OUTPUT);

		// Make sure the output folder exists
		fs::create_directories(comOutput);

		m_wolf.GetCommonEvents().ToJson(comOutput);

		std::cout << "Done" << std::endl;
	}

	void gameDat2Json() const
	{
		if (m_skipGD) return;

		std::cout << "Writing GameDat to JSON ... " << std::flush;

		const tString gameDatOutput = std::format(TEXT("{}/{}"), m_outputPath, OUTPUT_DIR);

		m_wolf.GetGameDat().ToJson(gameDatOutput);

		std::cout << "Done" << std::endl;
	}

	void patchMaps(const tString& patchFolder)
	{
		std::cout << "Patching Maps ... " << std::flush;

		const tString mapPatch = std::format(TEXT("{}/{}"), patchFolder, MAP_OUTPUT);

		// Check if the patch folder exists
		if (!fs::exists(mapPatch))
		{
			std::cerr << ERROR_TAG << "Map patch folder does not exist" << std::endl;
			return;
		}

		for (Map& map : m_wolf.GetMaps())
			map.Patch(mapPatch);

		std::cout << "Done" << std::endl;
	}

	void patchDatabases(const tString& patchFolder)
	{
		std::cout << "Patching Databases ... " << std::flush;

		const tString dbPatch = std::format(TEXT("{}/{}"), patchFolder, DB_OUTPUT);

		// Check if the patch folder exists
		if (!fs::exists(dbPatch))
		{
			std::cerr << ERROR_TAG << "Database patch folder does not exist" << std::endl;
			return;
		}

		for (Database& db : m_wolf.GetDatabases())
			db.Patch(dbPatch);

		std::cout << "Done" << std::endl;
	}

	void patchCommonEvents(const tString& patchFolder)
	{
		std::cout << "Patching CommonEvents ... " << std::flush;

		const tString comPatch = std::format(TEXT("{}/{}"), patchFolder, COM_OUTPUT);

		// Check if the patch folder exists
		if (!fs::exists(comPatch))
		{
			std::cerr << ERROR_TAG << "Common event patch folder does not exist" << std::endl;
			return;
		}

		m_wolf.GetCommonEvents().Patch(comPatch);

		std::cout << "Done" << std::endl;
	}

	void patchGameDat(const tString& patchFolder)
	{
		if (m_skipGD) return;

		std::cout << "Patching GameDat ... " << std::flush;

		const tString gameDatPatch = std::format(TEXT("{}/{}"), patchFolder, OUTPUT_DIR);

		m_wolf.GetGameDat().Patch(gameDatPatch);

		std::cout << "Done" << std::endl;
	}

private:
	tString m_dataPath;
	tString m_outputPath;
	WolfRPG m_wolf;
	bool m_skipGD;
};

int main(int argc, char* argv[])
{
	EnableUTF8Print();

	std::cout << "WolfTL v" << VERSION << std::endl;

	if (argc < 4)
	{
		std::cout << "Usage: " << argv[0] << " <DATA-FOLDER> <OUTPUT-FOLDER> <MODE> [OPTION]" << std::endl;
		std::cout << "Modes:" << std::endl;
		std::cout << "  create    - Create the Patch" << std::endl;
		std::cout << "  patch     - Apply the Patch" << std::endl;
		std::cout << "  patch_ip  - Apply the Patch in place, i.e., override the original data files" << std::endl;
		std::cout << "Options:" << std::endl;
		std::cout << "  no_gd     - Skip Game.dat (currently always active)" << std::endl;
		return 0;
	}

	LPWSTR* szArglist;
	int32_t nArgs;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (szArglist == nullptr)
	{
		std::cout << "CommandLineToArgvW failed" << std::endl;
		return -1;
	}

	const tString dataFolder   = szArglist[1];
	const tString outputFolder = szArglist[2];
	tString mode               = szArglist[3];

	bool skipGameDat = true;

	// if (argc >= 5)
	//{
	//	if (tString(szArglist[4]) == TEXT("no_gd"))
	//		skipGameDat = true;
	// }

	// Convert to lowercase
	std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

	LocalFree(szArglist);

	try
	{
		WolfTL wolf(dataFolder, outputFolder, skipGameDat);
		if (mode == TEXT("create"))
			wolf.ToJson();
		else if (mode == TEXT("patch"))
			wolf.Patch();
		else if (mode == TEXT("patch_ip"))
			wolf.Patch(true);
		else
			std::wcerr << L"Invalid mode: " << mode << std::endl;
	}
	catch (const std::exception& e)
	{
		std::wcerr << std::endl
				   << "Error while processing: " << g_activeFile << std::endl
				   << e.what() << std::endl;
	}

	return 0;
}

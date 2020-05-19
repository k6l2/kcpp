#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <vector>
#include <cassert>
#include <chrono>
namespace chrono = std::chrono;
namespace fs = std::filesystem;
static bool g_verbose;
static bool writeEntireFile(const fs::path::value_type* fileName, 
                            const char* nullTerminatedFileData)
{
#if _MSC_VER
	FILE* file = _wfopen(fileName, L"wb");
#else
	FILE* file = fopen(fileName, "wb");
#endif
	if(file)
	{
		const size_t fileDataByteCount = strlen(nullTerminatedFileData);
		const size_t bytesWritten = fwrite(nullTerminatedFileData, sizeof(char), 
		                                   fileDataByteCount, file);
		if(fclose(file) != 0)
		{
			fprintf(stderr, "Failed to close '%ws'!\n", fileName);
		}
		if(bytesWritten != fileDataByteCount)
		{
			fprintf(stderr, "Failed to write '%ws'!\n", fileName);
			return false;
		}
	}
	else
	{
		fprintf(stderr, "Failed to open '%ws'!\n", fileName);
		return false;
	}
	return true;
}
/** @return null-ternimated c-string of the entire file's contents */
static char* readEntireFile(const fs::path::value_type* fileName, 
                            uintmax_t fileSize)
{
#if _MSC_VER
	FILE* file = _wfopen(fileName, L"rb");
#else
	FILE* file = fopen(fileName, "rb");
#endif
	if(file)
	{
		char* data = static_cast<char*>(malloc(fileSize + 1));
		if(!data)
		{
			fprintf(stderr, "Failed to alloc %lli bytes for '%ws'!\n", 
			        fileSize + 1, fileName);
			return nullptr;
		}
		const size_t bytesRead = fread(data, sizeof(char), fileSize, file);
		if(bytesRead != fileSize)
		{
			free(data);
			fprintf(stderr, "Failed to completely read '%ws'!\n", fileName);
			return nullptr;
		}
		if(fclose(file) != 0)
		{
			fprintf(stderr, "Failed to close '%ws'!\n", fileName);
		}
		data[fileSize] = '\0';
		return data;
	}
	else
	{
		fprintf(stderr, "Failed to open '%ws'!\n", fileName);
		return nullptr;
	}
}
static void printManual()
{
	printf("---KC++: An extremely lightweight language extension to C++ ---\n");
	printf("Usage: kc++ input_code_tree_directory "
	       "desired_output_directory [--verbose]\n");
	printf("Result: Upon successful completion, all C++ code in the input \n"
	       "\tcode tree directory is transformed and saved into the desired \n"
	       "\toutput directory.\n");
	printf("Transformations: \n");
	printf("\t-Replace `KASSET(c_str)` macros with an array index unique to \n"
	       "\t    this c_str.\n");
	printf("\t-Generate a file at the root of desired output directory \n"
	       "\t    called `kasset_array.h`, which contains the accumulated \n"
	       "\t    string data of all KASSET macro invocations in an array.\n");
}
int main(int argc, char** argv)
{
	const auto timeMainStart = chrono::high_resolution_clock::now();
	if(argc < 3)
	{
		fprintf(stderr, "ERROR: incorrect usage!\n");
		printManual();
		return EXIT_FAILURE;
	}
	g_verbose = false;
	const fs::path inputCodeTreeDirectory = argv[1];
	const fs::path desiredOutputDirectory = argv[2];
	for(int a = 3; a < argc; a++)
	{
		if(strcmp(argv[a], "--verbose") == 0)
		{
			g_verbose = true;
		}
		else
		{
			fprintf(stderr, "ERROR: incorrect usage on param[%i]=='%s'\n",
			        a, argv[a]);
			printManual();
			return EXIT_FAILURE;
		}
	}
	if(g_verbose)
	{
		printf("inputCodeTreeDirectory='%ws'\n", 
		       inputCodeTreeDirectory.c_str());
		printf("desiredOutputDirectory='%ws'\n", 
		       desiredOutputDirectory.c_str());
	}
	for(const fs::directory_entry& p : 
		fs::recursive_directory_iterator(inputCodeTreeDirectory))
	{
		if(g_verbose)
		{
			printf("'%ws'\n", p.path().c_str());
		}
		if(p.is_directory())
		{
		}
		else if(p.is_regular_file())
		{
#if 0
			if(g_verbose)
			{
				printf("p.path().parent_path()='%ws' "
				       "p.path().filename()='%ws'\n",
				       p.path().parent_path().c_str(), 
				       p.path().filename().c_str());
			}
#endif
			// Create a path to the file excluding `inputCodeTreeDirectory` //
			fs::path inCodeTreePath = p.path().filename();
			{
				fs::path currPath = p.path().parent_path();
				while(currPath != inputCodeTreeDirectory)
				{
					inCodeTreePath = currPath.filename() / inCodeTreePath;
					currPath = currPath.parent_path();
				}
			}
#if 0
			if(g_verbose)
			{
				printf("inCodeTreePath='%ws'\n", inCodeTreePath.c_str());
			}
#endif
			// Create the corresponding file inside `desiredOutputDirectory` //
			const fs::path outPath = desiredOutputDirectory / inCodeTreePath;
			fs::create_directories(outPath.parent_path());
#if 0
			if(g_verbose)
			{
				printf("file size=%lli\n", fs::file_size(p.path()));
			}
#endif
			char*const fileData = 
				readEntireFile(p.path().c_str(), fs::file_size(p.path()));
			if(fileData)
			{
				if(!writeEntireFile(outPath.c_str(), fileData))
				{
					fprintf(stderr, "Failed to write file '%ws'!\n", 
					        outPath.c_str());
				}
				free(fileData);
			}
			else
			{
				fprintf(stderr, "Failed to read file '%ws'!\n", 
				        p.path().c_str());
			}
		}
	}
	// calculate the program execution time //
	const auto timeMainEnd = chrono::high_resolution_clock::now();
	const auto timeMainDuration = chrono::duration_cast<chrono::microseconds>(
		timeMainEnd - timeMainStart);
	const float secondsMainDuration = timeMainDuration.count() / 1000000.f;
	printf("kc++ complete! Seconds elapsed=%f\n", secondsMainDuration);
	return EXIT_SUCCESS;
}
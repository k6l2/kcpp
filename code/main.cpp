#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <vector>
using std::vector;
#include <chrono>
#include <string>
using std::string;
#include <sstream>
using std::stringstream;
#include <set>
using std::set;
#include <map>
using std::map;
namespace chrono = std::chrono;
namespace fs = std::filesystem;
static bool g_verbose;
#if KASSET_IMPLEMENTATION
static vector<string> g_kassets;
#endif// KASSET_IMPLEMENTATION
using PolymorphicTaggedUnionPureVirtualFunctionIdentifier = string;
struct PolymorphicTaggedUnionPureVirtualFunctionMetaData
{
	vector<string> qualifierTokens;
	vector<string> paramTokens;
};
struct PolymorphicTaggedUnionMetaData
{
	set<string> derivedStructIdentifierSet;
	map<PolymorphicTaggedUnionPureVirtualFunctionIdentifier, 
	    PolymorphicTaggedUnionPureVirtualFunctionMetaData> virtualFunctions;
};
using TaggedUnionStructIdentifier = string;
static map<TaggedUnionStructIdentifier, PolymorphicTaggedUnionMetaData> 
	g_polyTaggedUnions;
#include "tokenizer.cpp"
#define PARSE_FAILURE() \
	{ fprintf(stderr, "parse failure!\n");\
	  assert(false);\
	  exit(-1);\
	  return; }
static void 
	kcppParsePolymorphicTaggedUnion(KTokenizer& tokenizer)
{
	/* parse the `struct` keyword */
	{
		const KToken tokenStruct = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenStruct.type != KTokenType::IDENTIFIER)
			PARSE_FAILURE();
		const string tokenStr = 
			string(tokenStruct.text, tokenStruct.textLength);
		if(tokenStr != "struct")
			PARSE_FAILURE();
	}
	/* parse the struct identifier.  Using this string, we can start building a 
		list of structures which derive from this one.  We can also create a 
		file which must be included to add the union of derived structures to 
		this struct.  */
	{
		const KToken tokenStructId = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenStructId.type != KTokenType::IDENTIFIER)
			PARSE_FAILURE();
		const string tokenStr = 
			string(tokenStructId.text, tokenStructId.textLength);
		g_polyTaggedUnions.insert({tokenStr, {}});
	}
}
static void 
	kcppParsePolymorphicTaggedUnionExtension(KTokenizer& tokenizer)
{
	/* parse the parenthesis */
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_OPEN).type != 
			KTokenType::PAREN_OPEN)
		PARSE_FAILURE();
	/* parse the tagged union struct identifier */
	string parentStructId;
	{
		const KToken tokenStructId = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenStructId.type != KTokenType::IDENTIFIER)
			PARSE_FAILURE();
		parentStructId = string(tokenStructId.text, tokenStructId.textLength);
	}
	/* parse the closing parenthesis */
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_CLOSE).type != 
			KTokenType::PAREN_CLOSE)
		PARSE_FAILURE();
	/* parse the `struct` keyword */
	{
		const KToken tokenStruct = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenStruct.type != KTokenType::IDENTIFIER)
			PARSE_FAILURE();
		const string tokenStr = 
			string(tokenStruct.text, tokenStruct.textLength);
		if(tokenStr != "struct")
			PARSE_FAILURE();
	}
	/* parse the struct identifier */
	{
		const KToken tokenStructId = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenStructId.type != KTokenType::IDENTIFIER)
			PARSE_FAILURE();
		const string tokenStr = 
			string(tokenStructId.text, tokenStructId.textLength);
		/* now we can add this to the accumulated tagged union struct 
			relationships */
		auto ptuIt = g_polyTaggedUnions.find(parentStructId);
		if(ptuIt == g_polyTaggedUnions.end())
		{
			g_polyTaggedUnions.insert({parentStructId, {}});
			ptuIt = g_polyTaggedUnions.find(parentStructId);
		}
		if(ptuIt->second.derivedStructIdentifierSet.contains(tokenStr))
			PARSE_FAILURE();
		ptuIt->second.derivedStructIdentifierSet.insert(tokenStr);
	}
}
static void 
	kcppParsePolymorphicTaggedUnionPureVirtualFunctionDefinition(
		KTokenizer& tokenizer)
{
	/* continue parsing identifier tokens until we reach an open parenthesis,
		storing the function identifier strings as we go */
	string functionIdentifier;
	vector<string> functionQualifiers;
	KTokenType lastFunctionQualifierTokenType;
	for(;;)
	{
		const KToken token = ktokeNext(tokenizer);
		if(token.type == KTokenType::PAREN_OPEN)
		/* once we reach an open paren, we know the last identifier was the 
			function name */
		{
			assert(lastFunctionQualifierTokenType == KTokenType::IDENTIFIER);
			functionIdentifier = functionQualifiers.back();
			functionQualifiers.pop_back();
			break;
		}
		functionQualifiers.push_back(string(token.text, token.textLength));
		lastFunctionQualifierTokenType = token.type;
	}
	/* continue parsing tokens until we reach a close paren, storing all the 
		tokens as we go */
	string ownerPtuIdentifier;
	vector<string> functionParamTokens;
	for(;;)
	{
		const KToken token = ktokeNext(tokenizer);
		if(token.type == KTokenType::PAREN_CLOSE)
		{
			break;
		}
		functionParamTokens.push_back(string(token.text, token.textLength));
		/* the first identifier is required to be the struct identifier of the 
			PTU which owns this function */
		if(functionParamTokens.size() == 1)
		{
			assert(token.type == KTokenType::IDENTIFIER);
			ownerPtuIdentifier = functionParamTokens.back();
		}
	}
	/* save this pure virtual function declaration inside g_polyTaggedUnions so 
		we can generate dispatch code to automatically call functions which 
		override this */
	auto ptuIt = g_polyTaggedUnions.find(ownerPtuIdentifier);
	if(ptuIt == g_polyTaggedUnions.end())
	{
		g_polyTaggedUnions.insert({ownerPtuIdentifier, {}});
		ptuIt = g_polyTaggedUnions.find(ownerPtuIdentifier);
	}
	/* ensure that there is only ONE virtual function with this identifier 
		declared for this polymorphic tagged union! */
	auto pvfIt = ptuIt->second.virtualFunctions.find(functionIdentifier);
	assert(pvfIt == ptuIt->second.virtualFunctions.end());
	/* finally, we can add the extracted function declaration to the virtual 
		function set of this PTU */
	ptuIt->second.virtualFunctions[functionIdentifier] = 
		{ .qualifierTokens = functionQualifiers
		, .paramTokens     = functionParamTokens };
}
#if KASSET_IMPLEMENTATION
static void kcppParseKAssetInclude(KTokenizer& tokenizer, string& outString)
{
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_OPEN).type == 
		KTokenType::PAREN_OPEN)
	{
		outString.append("#include \"gen_kassets.h\"");
		kcppRequireToken(tokenizer, KTokenType::PAREN_CLOSE);
	}
	else
	{
		fprintf(stderr, "kcppParseKAssetInclude failed!\n"); assert(false); 
	}
}
static void kcppParseKAsset(KTokenizer& tokenizer, string& outString)
{
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_OPEN).type == 
		KTokenType::PAREN_OPEN)
	{
		KToken tokenAssetString = 
			kcppRequireToken(tokenizer, KTokenType::STRING);
		if(tokenAssetString.type == KTokenType::STRING)
		{
			const string strKasset(tokenAssetString.text, 
			                       tokenAssetString.textLength);
			auto itKasset = find(g_kassets.begin(), g_kassets.end(), strKasset);
			size_t kAssetIndex = g_kassets.size();
			if(itKasset == g_kassets.end())
			{
				g_kassets.push_back(strKasset);
			}
			else
			{
				kAssetIndex = itKasset - g_kassets.begin();
			}
			stringstream ss;
			ss << "&g_kassets[" << kAssetIndex << "]";
			outString.append(ss.str());
			kcppRequireToken(tokenizer, KTokenType::PAREN_CLOSE);
		}
		else
		{
			fprintf(stderr, "kcppParseKAsset failed 2!\n"); assert(false); 
		}
	}
	else
	{
		fprintf(stderr, "kcppParseKAsset failed!\n"); assert(false); 
	}
}
static void kcppParseKAssetSearch(KTokenizer& tokenizer, string& outString)
{
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_OPEN).type == 
		KTokenType::PAREN_OPEN)
	{
		KToken tokenAssetFileNamePu8 = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenAssetFileNamePu8.type == KTokenType::IDENTIFIER)
		{
			const string strPu8(tokenAssetFileNamePu8.text, 
			                    tokenAssetFileNamePu8.textLength);
			stringstream ss;
			ss << "findKAssetCStr(" << strPu8 << ")";
			outString.append(ss.str());
			kcppRequireToken(tokenizer, KTokenType::PAREN_CLOSE);
		}
		else
		{
			fprintf(stderr, "kcppParseKAssetSearch failed 2!\n"); assert(false); 
		}
	}
	else
	{
		fprintf(stderr, "kcppParseKAssetSearch failed!\n"); assert(false); 
	}
}
static void kcppParseKAssetCStr(KTokenizer& tokenizer, string& outString)
{
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_OPEN).type == 
		KTokenType::PAREN_OPEN)
	{
		KToken tokenAssetIndex = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenAssetIndex.type == KTokenType::IDENTIFIER)
		{
			const string strSizeT(tokenAssetIndex.text, 
			                      tokenAssetIndex.textLength);
			stringstream ss;
			ss << "g_kassets[" << strSizeT << "]";
			outString.append(ss.str());
			kcppRequireToken(tokenizer, KTokenType::PAREN_CLOSE);
		}
		else
		{
			fprintf(stderr, "kcppParseKAssetCStr failed 2!\n"); assert(false); 
		}
	}
	else
	{
		fprintf(stderr, "kcppParseKAssetCStr failed!\n"); assert(false); 
	}
}
static void kcppParseKAssetIndex(KTokenizer& tokenizer, string& outString)
{
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_OPEN).type == 
		KTokenType::PAREN_OPEN)
	{
		KToken tokenAssetString = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenAssetString.type == KTokenType::IDENTIFIER)
		{
			const string strKAssetCStr(tokenAssetString.text, 
			                           tokenAssetString.textLength);
			stringstream ss;
			ss << "static_cast<u32>("<< strKAssetCStr << " - g_kassets)";
			outString.append(ss.str());
			kcppRequireToken(tokenizer, KTokenType::PAREN_CLOSE);
		}
		else
		{
			fprintf(stderr, "kcppParseKAsset failed 2!\n"); assert(false); 
		}
	}
	else
	{
		fprintf(stderr, "kcppParseKAsset failed!\n"); assert(false); 
	}
}
static void kcppParseKAssetType(KTokenizer& tokenizer, string& outString)
{
	if(kcppRequireToken(tokenizer, KTokenType::PAREN_OPEN).type == 
		KTokenType::PAREN_OPEN)
	{
		KToken tokenAssetString = 
			kcppRequireToken(tokenizer, KTokenType::IDENTIFIER);
		if(tokenAssetString.type == KTokenType::IDENTIFIER)
		{
			const string strKAssetCStr(tokenAssetString.text, 
			                           tokenAssetString.textLength);
			stringstream ss;
			ss << "g_kassetFileTypes[("<< strKAssetCStr << " - g_kassets)]";
			outString.append(ss.str());
			kcppRequireToken(tokenizer, KTokenType::PAREN_CLOSE);
		}
		else
		{
			fprintf(stderr, "kcppParseKAsset failed 2!\n"); assert(false); 
		}
	}
	else
	{
		fprintf(stderr, "kcppParseKAsset failed!\n"); assert(false); 
	}
}
static void kcppParseKAssetCount(KTokenizer& tokenizer, string& outString)
{
	outString.append("(sizeof(g_kassets)/sizeof(g_kassets[0]))");
}
static void kcppParseKAssetTypePng(KTokenizer& tokenizer, string& outString)
{
	outString.append("KAssetFileType::PNG");
}
static void kcppParseKAssetTypeWav(KTokenizer& tokenizer, string& outString)
{
	outString.append("KAssetFileType::WAV");
}
static void kcppParseKAssetTypeOgg(KTokenizer& tokenizer, string& outString)
{
	outString.append("KAssetFileType::OGG");
}
static void kcppParseKAssetTypeFlipbookMeta(KTokenizer& tokenizer, 
                                            string& outString)
{
	outString.append("KAssetFileType::FLIPBOOK_META");
}
static void kcppParseKAssetTypeUnknown(KTokenizer& tokenizer, string& outString)
{
	outString.append("KAssetFileType::UNKNOWN");
}
#endif// KASSET_IMPLEMENTATION
static void kcppParseMacroDefinition(KTokenizer& tokenizer, string& outString)
{
	const char* macroDef = tokenizer.at;
	size_t macroDefSize = 0;
	while(tokenizer.at[0] && !isEndOfLine(tokenizer.at[0]))
	{
		if(tokenizer.at[0] == '\\' && isEndOfLine(tokenizer.at[1]))
		{
			do
			{
				tokenizer.at++;
				macroDefSize++;
			} while (isEndOfLine(tokenizer.at[0]));
		}
		else
		{
			tokenizer.at++;
			macroDefSize++;
		}
	}
	outString.append(macroDef, macroDefSize);
}
static void processFileData(const char* fileData)
{
	//string result;
	bool parsing = true;
	KTokenizer tokenizer = {.at = fileData };
	while(parsing)
	{
		KToken token = ktokeNext(tokenizer);
#if 0
		printf("%d:'%.*s'\n", token.type, token.textLength, token.text);
		if( ktokeEquals(token, "stb_decompress") ||
			ktokeEquals(token, "proggy_clean_ttf_compressed_data_base85"))
		{
			fflush(stdout);
			printf("hello");
		}
#endif// 0
		switch(token.type)
		{
			case KTokenType::HASH_TAG:
			{
				KToken tokenNext = ktokeNext(tokenizer);
//				result.append(token.text, token.textLength);
//				result.append(tokenNext.text, tokenNext.textLength);
				if(tokenNext.type == KTokenType::IDENTIFIER &&
					ktokeEquals(tokenNext, "define"))
				{
					string macroDefinition;
					kcppParseMacroDefinition(tokenizer, macroDefinition);
				}
			}break;
			case KTokenType::IDENTIFIER:
			{
				if(ktokeEquals(token, "KCPP_POLYMORPHIC_TAGGED_UNION"))
				{
					kcppParsePolymorphicTaggedUnion(tokenizer);
				}
				if(ktokeEquals(token, "KCPP_POLYMORPHIC_TAGGED_UNION_EXTENDS"))
				{
					kcppParsePolymorphicTaggedUnionExtension(tokenizer);
				}
				if(ktokeEquals(
					token, "KCPP_POLYMORPHIC_TAGGED_UNION_PURE_VIRTUAL"))
				{
					kcppParsePolymorphicTaggedUnionPureVirtualFunctionDefinition(tokenizer);
				}
#if KASSET_IMPLEMENTATION
				if(ktokeEquals(token, "INCLUDE_KASSET"))
				{
					kcppParseKAssetInclude(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET"))
				{
					kcppParseKAsset(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_SEARCH"))
				{
					kcppParseKAssetSearch(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_CSTR"))
				{
					kcppParseKAssetCStr(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_INDEX"))
				{
					kcppParseKAssetIndex(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_TYPE"))
				{
					kcppParseKAssetType(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_COUNT"))
				{
					kcppParseKAssetCount(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_TYPE_PNG"))
				{
					kcppParseKAssetTypePng(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_TYPE_WAV"))
				{
					kcppParseKAssetTypeWav(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_TYPE_OGG"))
				{
					kcppParseKAssetTypeOgg(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_TYPE_FLIPBOOK_META"))
				{
					kcppParseKAssetTypeFlipbookMeta(tokenizer, result);
				}
				else if(ktokeEquals(token, "KASSET_TYPE_UNKNOWN"))
				{
					kcppParseKAssetTypeUnknown(tokenizer, result);
				}
				else
#endif// KASSET_IMPLEMENTATION
				{
//					result.append(token.text, token.textLength);
				}
			}break;
			case KTokenType::STRING:
			{
//				result.push_back('"');
//				result.append(token.text, token.textLength);
//				result.push_back('"');
			}break;
			case KTokenType::CHARACTER:
			{
//				result.push_back('\'');
//				result.append(token.text, token.textLength);
//				result.push_back('\'');
			}break;
			default:
			{
//				result.append(token.text, token.textLength);
			}break;
			case KTokenType::END_OF_STREAM:
			{
				parsing = false;
			}break;
		}
	}
}
#if defined(_WIN32)
#include <Windows.h>
#if CLONE_FILE_TIMESTAMPS
void cloneFileTimestamps(const std::filesystem::path& source, 
                         const std::filesystem::path& dest)
{
	const HANDLE hFileSource = 
		CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, 
		            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	assert(hFileSource != INVALID_HANDLE_VALUE);
	if(hFileSource == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to open file '%ws'! getlasterror=%i\n", 
		        source.c_str(), GetLastError());
	}
	const HANDLE hFileDest = 
		CreateFileW(dest.c_str(), FILE_WRITE_ATTRIBUTES, 0, nullptr, 
		            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	assert(hFileDest != INVALID_HANDLE_VALUE);
	if(hFileDest == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to open file '%ws'! getlasterror=%i\n", 
		        dest.c_str(), GetLastError());
	}
	FILETIME fileTimeCreation;
	FILETIME fileTimeLastAccess;
	FILETIME fileTimeLastWrite;
	if(!GetFileTime(hFileSource, &fileTimeCreation, &fileTimeLastAccess, 
	                &fileTimeLastWrite))
	{
		fprintf(stderr, "Failed to get file timestamps for '%ws'! "
		        "getlasterror=%i\n", source.c_str(), GetLastError());
	}
	if(!SetFileTime(hFileDest, &fileTimeCreation, &fileTimeLastAccess, 
	                &fileTimeLastWrite))
	{
		fprintf(stderr, "Failed to set file timestamps for '%ws'! "
		        "getlasterror=%i\n", dest.c_str(), GetLastError());
	}
	CloseHandle(hFileDest);
	CloseHandle(hFileSource);
}
static void setFileReadOnly(const fs::path::value_type* fileName)
{
	if(!SetFileAttributesW(fileName, FILE_ATTRIBUTE_READONLY))
	{
		fprintf(stderr, "Failed to set read-only '%ws'!\n", fileName);
	}
}
#endif// defined(_WIN32)
#endif// CLONE_FILE_TIMESTAMPS
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
#if KASSET_IMPLEMENTATION
string generateHeaderKAssets()
{
	string result;
	result.append("#pragma once\n");
	result.append("static const char* g_kassets[] = {\n");
	if(g_kassets.empty())
	{
		result.append("\t\"NO_KASSETS_FOUND_IN_SOURCE!\"\n");
	}
	for(size_t a = 0; a < g_kassets.size(); a++)
	{
		const string& kasset = g_kassets[a];
		stringstream ss;
		ss << "\t\"" << kasset<< "\",\n";
		result.append(ss.str());
		const size_t fbmPostfixOffset = kasset.find(".fbm", kasset.size() - 4);
		if(fbmPostfixOffset != string::npos)
		{
			// Implicitly infer that there must be a `png` file of the same file
			//	name prefix in the same directory as the `fbm`! (for now...) //
			const string impliedPngAssetFileName = 
				kasset.substr(0, fbmPostfixOffset) + string(".png");
			auto itImpliedPng = find(g_kassets.begin(), g_kassets.end(), 
			                         impliedPngAssetFileName);
			if(itImpliedPng == g_kassets.end())
			{
				g_kassets.push_back(impliedPngAssetFileName);
			}
		}
	}
	result.append("};\n");
	result.append("enum class KAssetFileType : unsigned char {\n");
	result.append("\tPNG,\n");
	result.append("\tWAV,\n");
	result.append("\tOGG,\n");
	result.append("\tFLIPBOOK_META,\n");
	result.append("\tUNKNOWN,\n");
	result.append("};\n");
	result.append("static const KAssetFileType g_kassetFileTypes[] = {\n");
	if(g_kassets.empty())
	{
		result.append("\tKAssetFileType::UNKNOWN\n");
	}
	for(const string& kasset : g_kassets)
	{
		if(kasset.find(".png", kasset.size() - 4) != string::npos)
		{
			result.append("\tKAssetFileType::PNG,\n");
		}
		else if(kasset.find(".wav", kasset.size() - 4) != string::npos)
		{
			result.append("\tKAssetFileType::WAV,\n");
		}
		else if(kasset.find(".ogg", kasset.size() - 4) != string::npos)
		{
			result.append("\tKAssetFileType::OGG,\n");
		}
		else if(kasset.find(".fbm", kasset.size() - 4) != string::npos)
		{
			result.append("\tKAssetFileType::FLIPBOOK_META,\n");
		}
		else
		{
			result.append("\tKAssetFileType::UNKNOWN,\n");
		}
	}
	result.append("};\n");
	result.append("static const char*const* findKAssetCStr(const char* str)\n");
	result.append("{\n");
	result.append("\tfor(size_t a = 0; \n"
	              "\t    a < (sizeof(g_kassets)/sizeof(g_kassets[0])); a++)\n");
	result.append("\t{\n");
	result.append("\t\tif(strcmp(g_kassets[a], str) == 0)\n");
	result.append("\t\t{\n");
	result.append("\t\t\treturn &g_kassets[a];\n");
	result.append("\t\t}\n");
	result.append("\t}\n");
	result.append("\treturn nullptr;\n");
	result.append("}\n");
	return result;
}
#endif// KASSET_IMPLEMENTATION
static void printManual()
{
	printf("---KCpp: An extremely lightweight language extension to C++ ---\n");
	printf("Usage: kcpp input_code_tree_directories generated_code_directory "
	       "[--verbose]\n");
	printf("@param input_code_tree_directories: A semicolon-separated list of "
	       "directories containing C++ code which needs to be processed by "
	       "metaprogramming routines.\n");
#if 0
	printf("Result: Upon successful completion, all C++ code in the input \n"
	       "\tcode tree directory is transformed and saved into the desired \n"
	       "\toutput directory.\n");
	printf("Transformations: \n");
	printf("\t-Replace `KASSET(c_str)` macros with an array index unique to \n"
	       "\t    this c_str.\n");
	printf("\t-Generate a file at the root of desired output directory \n"
	       "\t    called `kasset_array.h`, which contains the accumulated \n"
	       "\t    string data of all KASSET macro invocations in an array.\n");
#endif// 0
}
vector<string> 
	split(char *phrase, string delimiter)
{
	/* source: https://stackoverflow.com/a/44495206/4526664 */
	vector<string> list;
	string s = string(phrase);
	size_t pos = 0;
	string token;
	while ((pos = s.find(delimiter)) != string::npos) 
	{
		token = s.substr(0, pos);
		list.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	list.push_back(s);
	return list;
}
vector<fs::path> 
	vecStringToVecFsPath(const vector<string>& vecString)
{
	vector<fs::path> result;
	result.reserve(vecString.size());
	for(const string& s : vecString)
	{
		result.push_back(s);
	}
	return result;
}
static string 
	generatePolymorphicTaggedUnionIncludes(
		const string& ptuIdentifier, 
		const PolymorphicTaggedUnionMetaData& ptuMeta)
{
	string result;
	result.append("#pragma once\n");
	for(const string& ptuDerivedId : ptuMeta.derivedStructIdentifierSet)
	{
		string ptuDerivedIdCamelCase = ptuDerivedId;
		ptuDerivedIdCamelCase[0] = tolower(ptuDerivedId[0]);
		result.append("#include \"" + ptuDerivedIdCamelCase + ".h\"");
	}
	return result;
}
static string 
	toUpperCase(string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}
static string 
	generatePolymorphicTaggedUnion(
		const string& ptuIdentifier, 
		const PolymorphicTaggedUnionMetaData& ptuMeta)
{
	string result;
	/* generate a type enumeration for the tagged union */
	result.append("enum class Type : u16\n");
	result.append("	{ ");
	bool firstEnum = true;
	for(const string& ptuDerivedId : ptuMeta.derivedStructIdentifierSet)
	{
		if(!firstEnum)
			result.append("\n	, ");
		firstEnum = false;
		result.append(toUpperCase(ptuDerivedId));
	}
	if(!ptuMeta.derivedStructIdentifierSet.empty())
		result.append("\n	, ");
	result.append("ENUM_COUNT };\n");
	/* generate the union of derived structs */
	result.append("union\n");
	result.append("{\n");
	if(ptuMeta.derivedStructIdentifierSet.empty())
		result.append("	void* no_derived_structs;\n");
	else
		for(const string& ptuDerivedId : ptuMeta.derivedStructIdentifierSet)
		{
			string ptuDerivedIdTitleCase = ptuDerivedId;
			string ptuDerivedIdCamelCase = ptuDerivedId;
			ptuDerivedIdTitleCase[0] = toupper(ptuDerivedId[0]);
			ptuDerivedIdCamelCase[0] = tolower(ptuDerivedId[0]);
			result.append("	" + ptuDerivedIdTitleCase + " " + 
			              ptuDerivedIdCamelCase + ";\n");
		}
	result.append("};\n");
	return result;
}
int 
	main(int argc, char** argv)
{
	const auto timeMainStart = chrono::high_resolution_clock::now();
	if(argc < 2)
	{
		fprintf(stderr, "ERROR: incorrect usage!\n");
		printManual();
		return EXIT_FAILURE;
	}
	g_verbose = false;
	const vector<fs::path> vecFsPathInputs = 
		vecStringToVecFsPath(split(argv[1], ";"));
	const fs::path fsPathOutput = argv[2];
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
		for(int i = 0; i < vecFsPathInputs.size(); i++)
		{
			const fs::path fsPathInput = vecFsPathInputs[i];
			printf("input[%i]='%ws'\n", i, fsPathInput.c_str());
		}
		printf("output='%ws'\n", fsPathOutput.c_str());
	}
	/* parse all the files recursively in all the provided input directories */
	/* @TODO: ignore files that aren't C++ */
	int result = EXIT_SUCCESS;
	for(const fs::path& fsPathInput : vecFsPathInputs)
	{
		for(const fs::directory_entry& fsDirEnt : 
			fs::recursive_directory_iterator(fsPathInput))
		{
			if(fsDirEnt.is_directory())
				continue;
			if(!fsDirEnt.is_regular_file())
				continue;
			if(g_verbose)
				printf("kcpp('%ws')\n", fsDirEnt.path().c_str());
			char*const fileData = 
				readEntireFile(
					fsDirEnt.path().c_str(), fs::file_size(fsDirEnt.path()));
			if(fileData)
			{
				processFileData(fileData);
				free(fileData);
			}
			else
			{
				fprintf(stderr, "Failed to read file '%ws'!\n", 
				        fsDirEnt.path().c_str());
				result = EXIT_FAILURE;
			}
		}
	}
	/* output generated code into the provided output directory */
	fs::create_directories(fsPathOutput);
	for(const auto& ptu : g_polyTaggedUnions)
	{
		/* generate the code file which defines all pure virtual function 
			dispatchers declared for this PTU struct */
		{
			assert(!"TODO");
		}
		/* generate the code file which includes all the source files which 
			define the structures which make up the union within the PTU */
		{
			const string ptuGenFileNameIncludes = 
				"gen_ptu_" + ptu.first + "_includes.h";
			const fs::path outPathIncludes = 
				fsPathOutput / ptuGenFileNameIncludes;
			const string fileData = 
				generatePolymorphicTaggedUnionIncludes(ptu.first, ptu.second);
			if(!writeEntireFile(outPathIncludes.c_str(), fileData.c_str()))
			{
				fprintf(stderr, "Failed to write file '%ws'!\n", 
						outPathIncludes.c_str());
				result = EXIT_FAILURE;
			}
		}
		/* generate the code file which declares the anonomous union of the 
			PTU */
		{
			const string ptuGenFileName = "gen_ptu_" + ptu.first + ".h";
			const fs::path outPath = fsPathOutput / ptuGenFileName;
			const string fileData = 
				generatePolymorphicTaggedUnion(ptu.first, ptu.second);
			if(!writeEntireFile(outPath.c_str(), fileData.c_str()))
			{
				fprintf(stderr, "Failed to write file '%ws'!\n", 
						outPath.c_str());
				result = EXIT_FAILURE;
			}
		}
	}
#if 0
	const string tempInputCodeTreeFolderName = 
		inputCodeTreeDirectory.filename().string() + "_backup";
	const fs::path inputCodeTreeParentDirectory = inputCodeTreeDirectory/"..";
	const fs::path backupInputCodeTreeDirectory = 
		inputCodeTreeParentDirectory/tempInputCodeTreeFolderName;
	fs::rename(inputCodeTreeDirectory, backupInputCodeTreeDirectory);
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
			// Create a path to the file excluding 
			//	`backupInputCodeTreeDirectory` //
			fs::path inCodeTreePath = p.path().filename();
			{
				fs::path currPath = p.path().parent_path();
				while(currPath != backupInputCodeTreeDirectory)
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
			// Recreate the original source directory //
			const fs::path outPathOriginal = 
				inputCodeTreeDirectory / inCodeTreePath;
			fs::create_directories(outPathOriginal.parent_path());
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
				const string processedFileData = processFileData(fileData);
				free(fileData);
				// Recreate the original file, with kc++ modifications applied //
				if(!writeEntireFile(outPathOriginal.c_str(), 
				                    processedFileData.c_str()))
				{
					fprintf(stderr, "Failed to write file '%ws'!\n", 
					        outPathOriginal.c_str());
				}
				cloneFileTimestamps(p, outPathOriginal);
			}
			else
			{
				fprintf(stderr, "Failed to read file '%ws'!\n", 
				        p.path().c_str());
			}
		}
	}
#if KASSET_IMPLEMENTATION
	// generate the kasset string database //
	if(!g_kassets.empty())
	{
		const fs::path outPath = inputCodeTreeDirectory / "gen_kassets.h";
		const string fileData = generateHeaderKAssets();
		if(!writeEntireFile(outPath.c_str(), fileData.c_str()))
		{
			fprintf(stderr, "Failed to write file '%ws'!\n", 
					outPath.c_str());
		}
		setFileReadOnly(outPath.c_str());
	}
#endif// KASSET_IMPLEMENTATION
#endif// 0
	// calculate the program execution time //
	const auto timeMainEnd = chrono::high_resolution_clock::now();
	const auto timeMainDuration = chrono::duration_cast<chrono::microseconds>(
		timeMainEnd - timeMainStart);
	const float secondsMainDuration = timeMainDuration.count() / 1000000.f;
	printf("kcpp complete! Seconds elapsed=%f\n", secondsMainDuration);
	return result;
}
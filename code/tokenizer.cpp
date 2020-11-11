#include <cstdint>
enum class KTokenType : int8_t
	{ PAREN_OPEN
	, PAREN_CLOSE
	, COLON
	, COMMA
	, SEMICOLON
	, ASTERISK
	, BRACKET_OPEN
	, BRACKET_CLOSE
	, BRACE_OPEN
	, BRACE_CLOSE
	, HASH_TAG
	, WHITESPACE
	, COMMENT
	, STRING
	, CHARACTER
	, IDENTIFIER
	, END_OF_STREAM
	, UNKNOWN };
struct KToken
{
	KTokenType type;
	int textLength;
	const char* text;
};
struct KTokenizer
{
	const char* at;
};
static bool isEndOfLine(char c)
{
	const bool result = 
		c == '\r' ||
		c == '\n';
	return result;
}
static bool isWhitespace(char c)
{
	const bool result = 
		c == ' ' ||
		c == '\t' ||
		isEndOfLine(c);
	return result;
}
static KToken ktokeParseWhitespace(KTokenizer& tokenizer)
{
	KToken result = {
		.type = KTokenType::WHITESPACE,
		.textLength = 0,
		.text = tokenizer.at
	};
	while(isWhitespace(tokenizer.at[0]))
	{
		tokenizer.at++;
		result.textLength++;
	}
	return result;
}
static KToken ktokeParseComment(KTokenizer& tokenizer)
{
	KToken result = {
		.type = KTokenType::COMMENT,
		.textLength = 0,
		.text = tokenizer.at
	};
	if(tokenizer.at[0] == '/' && tokenizer.at[1] == '/')
	{
		tokenizer.at += 2;
		result.textLength += 2;
		while(tokenizer.at[0] && !isEndOfLine(tokenizer.at[0]))
		{
			tokenizer.at++;
			result.textLength++;
		}
		return result;
	}
	else if(tokenizer.at[0] == '/' && tokenizer.at[1] == '*')
	{
		tokenizer.at += 2;
		result.textLength += 2;
		while(tokenizer.at[0] && 
			!(tokenizer.at[0] == '*' && tokenizer.at[1] == '/'))
		{
			tokenizer.at++;
			result.textLength++;
		}
		if(tokenizer.at[0] == '*')
		{
			tokenizer.at += 2;
			result.textLength += 2;
		}
		return result;
	}
	return result;
}
static bool isAlpha(char c)
{
	const bool result = 
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '_';
	return result;
}
static bool isNumeric(char c)
{
	const bool result = (c >= '0' && c <= '9');
	return result;
}
KToken ktokeParseIdentifier(KTokenizer& tokenizer)
{
	KToken result = {
		.type = KTokenType::IDENTIFIER,
		.textLength = 1,
		.text = tokenizer.at
	};
	tokenizer.at++;
	while(isAlpha(tokenizer.at[0]) || isNumeric(tokenizer.at[0]))
	{
		tokenizer.at++;
		result.textLength++;
	}
	return result;
}
#if 0
KToken ktokeParseNumber(KTokenizer& tokenizer)
{
}
#endif // 0
static KToken ktokeNext(KTokenizer& tokenizer)
{
	if(isWhitespace(tokenizer.at[0]))
	{
		return ktokeParseWhitespace(tokenizer);
	}
	KToken result = {
		.type = KTokenType::UNKNOWN,
		.textLength = 1,
		.text = tokenizer.at
	};
	switch(tokenizer.at[0])
	{
		case '\0': result.type = KTokenType::END_OF_STREAM; tokenizer.at++; break;
		case '(':  result.type = KTokenType::PAREN_OPEN;    tokenizer.at++; break;
		case ')':  result.type = KTokenType::PAREN_CLOSE;   tokenizer.at++; break;
		case ':':  result.type = KTokenType::COLON;         tokenizer.at++; break;
		case ',':  result.type = KTokenType::COMMA;         tokenizer.at++; break;
		case ';':  result.type = KTokenType::SEMICOLON;     tokenizer.at++; break;
		case '*':  result.type = KTokenType::ASTERISK;      tokenizer.at++; break;
		case '[':  result.type = KTokenType::BRACKET_OPEN;  tokenizer.at++; break;
		case ']':  result.type = KTokenType::BRACKET_CLOSE; tokenizer.at++; break;
		case '{':  result.type = KTokenType::BRACE_OPEN;    tokenizer.at++; break;
		case '}':  result.type = KTokenType::BRACE_CLOSE;   tokenizer.at++; break;
		case '#':  result.type = KTokenType::HASH_TAG;      tokenizer.at++; break;
		case '/':
		{
			if(tokenizer.at[1] && 
				(tokenizer.at[1] == '/' || tokenizer.at[1] == '*'))
			{
				return ktokeParseComment(tokenizer);
			}
			// otherwise, this is something else like a division operator. Skip.
			tokenizer.at++;
		}break;
		case '"': 
		{
			result.type = KTokenType::STRING;
			result.textLength = 0;
			tokenizer.at++;
			result.text = tokenizer.at;
			while(tokenizer.at[0] && tokenizer.at[0] != '"')
			{
				if(tokenizer.at[0] == '\\' && tokenizer.at[1])
				{
					tokenizer.at++; result.textLength++;
				}
				tokenizer.at++; result.textLength++;
			}
			// consume the final '"' 
			if(tokenizer.at[0] == '"')
			{
				tokenizer.at++;
			}
		}break;
		case '\'':
		{
			result.type = KTokenType::CHARACTER;
			result.textLength = 0;
			tokenizer.at++;
			result.text = tokenizer.at;
			while(tokenizer.at[0] && tokenizer.at[0] != '\'')
			{
				if(tokenizer.at[0] == '\\' && tokenizer.at[1])
				{
					tokenizer.at++; result.textLength++;
				}
				tokenizer.at++; result.textLength++;
			}
			// consume the final '\''
			if(tokenizer.at[0] == '\'')
			{
				tokenizer.at++;
			}
		}break;
		default:
		{
			if(isAlpha(tokenizer.at[0]))
			{
				return ktokeParseIdentifier(tokenizer);
			}
#if 0
			else if(isNumeric(tokenizer.at[0]))
			{
				return ktokeParseNumber(tokenizer);
			}
#endif// 0
			else 
			{
				// skip our UNKNOWN token character
				tokenizer.at++;
			}
		}break;
	}
	return result;
}
static bool ktokeEquals(const KToken& token, const char* cStr)
{
	const char* cStrCurr = cStr;
	for(int c = 0; c < token.textLength; c++, cStrCurr++)
	{
		if(cStr[c] == '\0' || token.text[c] != cStr[c])
		{
			return false;
		}
	}
	const bool result = *cStrCurr == '\0';
	return result;
}
static KToken kcppRequireToken(KTokenizer& tokenizer, KTokenType tokenType)
{
	KToken token;
	do
	{
		token = ktokeNext(tokenizer);
#if 0
		if(token.type != tokenType &&
			(token.type == KTokenType::END_OF_STREAM ||
				!(token.type == KTokenType::WHITESPACE ||
				token.type == KTokenType::COMMENT)))
#endif// 0
		if(token.type == KTokenType::END_OF_STREAM)
		{
			return token;
		}
	} while(token.type != tokenType);
	return token;
}
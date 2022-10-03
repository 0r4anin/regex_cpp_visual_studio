#include "pch.h"
#include "CLogReader.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstdarg>


CLogReader::CLogReader(const bool *terminate)
	: m_hFile(INVALID_HANDLE_VALUE)
	, m_nPatterns(0)
	, m_nBufferBytes(0)
	, m_nBufferOffset(0)
	, m_terminate(terminate)
{
}

CLogReader::~CLogReader()
{
	Close();
	destroyPatterns();
}

bool CLogReader::Open(const char * fileName)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		Close();
	wchar_t path[MAX_PATH];
	int filePathLength = strnlen(fileName, MAX_PATH);
	if (filePathLength >= MAX_PATH)
	{
		ConsoleOut(__FUNCTION__, "File path length bigger then should! Must be less then %zu", MAX_PATH);
		return false;
	}
	
	MultiByteToWideChar(CP_ACP, 0, fileName, filePathLength+1, path, MAX_PATH);
	m_hFile = CreateFile(path,               // file to open
						GENERIC_READ,          // open for reading
						FILE_SHARE_READ,       // share for reading
						NULL,                  // default security
						OPEN_EXISTING,         // existing file only
						FILE_ATTRIBUTE_NORMAL, // normal file
						NULL);                 // no attr. template
	return m_hFile != INVALID_HANDLE_VALUE;
}

void CLogReader::Close()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	m_nBufferBytes = 0;
	m_nBufferOffset = 0;
}

bool CLogReader::SetFilter(const char * filter)
{
	if (filter == nullptr)
	{
		ConsoleOut(__FUNCTION__, "Can't set patterns filter is NULL");
		return false;
	}
	if (m_nPatterns > 0)
		destroyPatterns();
	
	const char* spec = nullptr;
	const char* comma = nullptr;
	char specSymbol = 0;
	int rangeMin = 0;
	int rangeMax = 0;
	
	while (m_nPatterns < MAX_PATTERN_LENGTH && *filter > 0x1F && *filter != 0x7F)
	{
		if (spec)
		{
			switch (specSymbol)
			{
			case '\\':
			{
				switch (*filter)
				{
				case '\\':
				case '*':
				case '?':
				case '[':
				case '{':
				case '+':
					m_patterns[m_nPatterns++] = new PatternSymbol(*filter);
					break;
				case 't':
					m_patterns[m_nPatterns++] = new PatternSymbol('\t');
					break;
				case 'd':
					m_patterns[m_nPatterns++] = new PatternSpecSymbol(PatternBase::PNum);
					break;
				case 'p':
					m_patterns[m_nPatterns++] = new PatternSpecSymbol(PatternBase::PAlpha);
					break;
				case 'w':
					m_patterns[m_nPatterns++] = new PatternSpecSymbol(PatternBase::PAlphaNum);
					break;
				default:
					ConsoleOut(__FUNCTION__, "Unknown spec characters %c%c", specSymbol, *filter);
					return false;
				}
				spec = nullptr;
				break;
			}
			case '[':
				//wait until end
				if (*filter == ']')
				{
					PatternSet *set = new PatternSet();
					m_patterns[m_nPatterns++] = set;
					if (!set->parseSet(spec + 1, filter))
					{
						ConsoleOut(__FUNCTION__, "Failed parse set of char \"%.*s\"", filter - spec + 1, spec);
						return false;
					}

					spec = nullptr;
					specSymbol = 0; 
				}
				break;
			case '{':
			{
				switch (*filter)
				{
				case '}':
				{
					if (!comma)
					{
						ConsoleOut(__FUNCTION__, "Failed parse set of char \"%.*s\"", filter - spec + 1, spec);
						return false;
					}

					char tmp[10] = { 0 };
					strncpy_s(tmp, spec + 1, comma - spec - 1);
					rangeMin = atoi(tmp);
					memset(tmp, 0, 10);
					strncpy_s(tmp, comma + 1, filter - comma - 1);
					rangeMax = atoi(tmp);
					m_patterns[m_nPatterns - 1] = new PatternRange(rangeMin, rangeMax, m_patterns[m_nPatterns - 1]);

					spec = nullptr;
					specSymbol = 0;
					break;
				}
				case ',':
					comma = filter;
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					break;
				default:
					return false;
				}
			
				break;
			}
			}
		}
		else
		{
			switch (*filter)
			{
			case '{':
				if (m_nPatterns == 0
					|| m_patterns[m_nPatterns - 1]->type == PatternBase::PSkip
					|| m_patterns[m_nPatterns - 1]->type == PatternBase::PRange)
				{
					return false;
				}
				comma = nullptr;
				//break; - not forgotten 
			case '[':
			case '\\':
				spec = filter;
				specSymbol = *filter;
				break;
			case '*':
				if (!(m_nPatterns > 0 && m_patterns[m_nPatterns-1]->type == PatternBase::PSkip))
					m_patterns[m_nPatterns++] = new PatternSkip();
				break;
			case '?':
				m_patterns[m_nPatterns++] = new PatternQuestion();
				break;
			case '+':
				//todo
				if (m_nPatterns == 0
					|| m_patterns[m_nPatterns - 1]->type == PatternBase::PSkip
					|| m_patterns[m_nPatterns - 1]->type == PatternBase::PRange)
				{
					return false;
				}
				m_patterns[m_nPatterns - 1] = new PatternRange(1, 0, m_patterns[m_nPatterns - 1]);
				break;
			default:
				m_patterns[m_nPatterns++] = new PatternSymbol(*filter);
				break;
			}
		}
		++filter;
	}

	if (*filter != 0)
	{
		ConsoleOut(__FUNCTION__, "Unknown spec characters 0x%x", *filter);
		return false;
	}

	return m_nPatterns > 0;
}

bool CLogReader::GetNextLine(char * buf, const int bufsize)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		ConsoleOut(__FUNCTION__, "Can't read line while file is closed");
		return false;
	}

	if (bufsize == 0)
	{
		ConsoleOut(__FUNCTION__, "Can't read line bufsize is zero");
		return false;
	}

	if (m_nPatterns == 0)
	{
		ConsoleOut(__FUNCTION__, "No filters found");
		return false;
	}

	char *cur = buf;
	char *end = buf + bufsize;

	resetPatterns();

	while (!(*m_terminate) && cur < end)
	{
		if (m_nBufferOffset >= m_nBufferBytes)
		{
			m_nBufferOffset = 0;
			m_nBufferBytes = 0;

			//read file
			if (ReadFile(m_hFile, m_buffer, READ_BUFFER_SIZE, &m_nBufferBytes, nullptr) == false || m_nBufferBytes == 0)
			{
				*cur = 0;
				if (cur != buf && MatchString(buf, cur, 0))
				{
					return true;
				}
				return false;
			}
		}
		switch (m_buffer[m_nBufferOffset])
		{
		case '\r'://only skip
			++m_nBufferOffset;
			break;
		case '\n':
			*cur = 0;
			++m_nBufferOffset;
			if (MatchString(buf, cur, 0))
				return true;
			cur = buf;
			resetPatterns();
			break;
		default:
			*cur = m_buffer[m_nBufferOffset++];
			++cur;
		}
	}

	if (cur >= end)
	{
		ConsoleOut(__FUNCTION__, "String to big");
	}

	return false;
}

void CLogReader::ConsoleOut(const char* function, const char * out, ...)
{
	va_list args;
	va_start(args, out);
	fprintf(stderr, "%s: ", function);
	vfprintf(stderr, out, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void CLogReader::destroyPatterns()
{
	for (int i = 0; i < m_nPatterns; ++i)
	{
		delete m_patterns[i];
	}
	m_nPatterns = 0;
}

void CLogReader::resetPatterns()
{
	for (int i = 0; i < m_nPatterns; ++i)
	{
		m_patterns[i]->Reset();
	}
}

bool CLogReader::MatchString(const char * cur, const char * end, int i)
{
	if (*m_terminate)
		return false;
	if (i >= m_nPatterns)
		return cur == end;
	if (cur < end)
	{
		if (!m_patterns[i]->Match(*cur))
		{
			m_patterns[i]->Reset();
			return false;
		}
		else
		{
			if ((m_patterns[i]->CanSkip() || m_patterns[i]->type == PatternBase::PRange) && MatchString(cur + 1, end, i))
			{
				return true;
			}

			if (MatchString(cur + 1, end, i + 1))
			{
				return true;
			}

			if (m_patterns[i]->CanSkip() && MatchString(cur, end, i + 1))
			{
				return true;
			}

			m_patterns[i]->Decrement();
			return false;
		}
		
	}
	else if (cur == end && (i + 1 == m_nPatterns) && m_patterns[i]->AllreadyMatched())
	{
		return true;
	}
	return false;
}

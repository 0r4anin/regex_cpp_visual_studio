#pragma once
#include <windows.h>

#include "Pattern.h"

#define MAX_PATTERN_LENGTH 2048
#define READ_BUFFER_SIZE 512

class CLogReader
{
public:
	CLogReader(const bool *terminate);
	~CLogReader();

	bool    Open(const char* fileName);                       // �������� �����, false - ������
	void    Close();                         // �������� �����

	bool    SetFilter(const char *filter);   // ��������� ������� �����, false - ������
	bool    GetNextLine(char *buf,           // ������ ��������� ��������� ������,
						const int bufsize);  // buf - �����, bufsize - ������������ �����
											 // false - ����� ����� ��� ������

private: //methods
	void ConsoleOut(const char* function, const char* out, ...);
	void destroyPatterns();
	void resetPatterns();

	bool MatchString(const char* cur, const char* end, int i);

private:
	HANDLE m_hFile;
	PatternBase *m_patterns[MAX_PATTERN_LENGTH];
	int m_nPatterns;
	char m_buffer[READ_BUFFER_SIZE];
	DWORD m_nBufferBytes;
	DWORD m_nBufferOffset;
	const bool* m_terminate;
};


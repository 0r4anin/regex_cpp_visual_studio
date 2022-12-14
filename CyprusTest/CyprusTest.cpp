// CyprusTest.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "CLogReader.h"
#include <stdio.h>

bool terminate =false;

BOOL CtrlHandler(DWORD fdwCtrlType)

{
	terminate = true; 
	
	return true;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
		fprintf(stderr, "Arguments count not matched. \n");
		fprintf(stderr, "usage: %s file_path pattern_to_line_match \n", argv[0]);
		fprintf(stderr, "       spec symbols for pattern ?*\\[{+ \n");
		fprintf(stderr, "       for match spec symbol use '\\' befor symbol, ex. '\\+' - means symbol '+' \n");
		fprintf(stderr, "\\d\t\t- match digit 0-9 \n");
		fprintf(stderr, "\\p\t\t- match alpha a-zA-Z \n");   
		fprintf(stderr, "\\w\t\t- match digit 0-9, alpha a-zA-Z and symbol '_' \n");
		fprintf(stderr, "[...]\t\t- set of range ex. 0-9, or symbols ex. [0-9}{.,<]\n");
		fprintf(stderr, "{min,max}\t- specify range of match ex. {1,3} match 1 or less then 4, max == 0 means infinity, failed with * \n");
		fprintf(stderr, "+\t\t- equal {1,0} \n");
		return 0;
	}
	
	CLogReader reader(&terminate);
	if (!reader.Open(argv[1]))
	{
		fprintf(stderr, "\n Failed open file \"%s\" ! \n", argv[1]);
		return 0;
	}

	if (!reader.SetFilter(argv[2]))
	{
		fprintf(stderr, "\n Failed set filter \"%s\" !\n", argv[2]);
		return 0;
	}

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true);

	char buffer[4096];
	while (!terminate && reader.GetNextLine(buffer, 4096))
	{
		printf("%s\n", buffer);
	}

	if (terminate)
	{
		fprintf(stderr, "\nCanceled by user.\n");
	}

	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.

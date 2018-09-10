// LPLab10.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
#include <locale>
#include <cwchar>

#include "Parm.h"
#include "Error.h"
#include "In.h"
#include "Log.h"
#include "FST.h"

int _tmain(int argc, _TCHAR ** argv)
{
	setlocale(LC_ALL, "rus");
	Log::LOG log = Log::INITLOG;
	try
	{
		Parm::PARM parm = Parm::getparm(argc, argv);
		In::IN in = In::getin(parm.in);
		log = Log::getlog(parm.log);
		Log::WriteLine(log, L"Разбор цепочек", L"");
		Log::WriteLog(log);
		Log::WriteParm(log, parm);
		//Logic here
		std::string fullSource(reinterpret_cast<char*>(in.text));
		std::string currentLine;
		size_t chainIdx{ 0 };
		size_t charIdx{ 0 };
		for (auto &chr : fullSource)
		{
			if (!(chr == '|' || chr == '\n'))
			{
				currentLine += chr;
				++charIdx;
			}
			else
			{
				FST::FST fst(currentLine.c_str(), 28,
					FST::NODE(1, FST::RELATION('b', 1)),								//0
					FST::NODE(1, FST::RELATION('e', 2)),								//1
					FST::NODE(1, FST::RELATION('g', 3)),								//2
					FST::NODE(1, FST::RELATION('i', 4)),								//3
					FST::NODE(1, FST::RELATION('n', 5)),								//4
					FST::NODE(1, FST::RELATION(';', 6)),								//5
					FST::NODE(2, FST::RELATION(' ', 6), FST::RELATION('r', 7)),			//6a
					FST::NODE(1, FST::RELATION('e', 8)),								//7
					FST::NODE(1, FST::RELATION('t', 9)),								//8
					FST::NODE(1, FST::RELATION('u', 10)),								//9
					FST::NODE(1, FST::RELATION('r', 11)),								//10
					FST::NODE(1, FST::RELATION('n', 12)),								//11
					FST::NODE(2, FST::RELATION(' ', 12), FST::RELATION('a', 13)),		//12
					FST::NODE(1, FST::RELATION('b', 14)),								//13
					FST::NODE(1, FST::RELATION('s', 15)),								//14

					FST::NODE(2, FST::RELATION('c', 16), FST::RELATION('p', 19)),		//15
					FST::NODE(1, FST::RELATION('a', 17)),								//16
					FST::NODE(1, FST::RELATION('l', 18)),								//17
					FST::NODE(1, FST::RELATION('c', 20)),								//18
					FST::NODE(1, FST::RELATION('r', 20)),								//19

					FST::NODE(1, FST::RELATION(';', 21)),								//20

					FST::NODE(3, FST::RELATION('a', 22), FST::RELATION('e', 24), FST::RELATION(' ', 23)),	//21
					FST::NODE(1, FST::RELATION('b', 14)),													//22

					FST::NODE(2, FST::RELATION(' ', 23), FST::RELATION('e', 24)),							//23
					FST::NODE(1, FST::RELATION('n', 25)),													//24
					FST::NODE(1, FST::RELATION('d', 26)),													//25
					FST::NODE(1, FST::RELATION(';', 27)),													//26

					FST::NODE()															//
				);
				wchar_t wChainIdx[24];
				wchar_t wCharIdx[24];
				_itow_s(chainIdx, wChainIdx, 10);
				_itow_s(charIdx, wCharIdx, 10);
				if (FST::execute(fst))
				{
					Log::WriteLine(log, L"Цепочка ", wChainIdx, L" распознана", L"");
				}
				else {
					Log::WriteLine(log, L"Цепочка ", wChainIdx, L" не распознана. Элемент: ", wCharIdx, L"");
				}
				currentLine.clear();
				++chainIdx;
			}
		}
		//LP10 contineous here
		Log::WriteIn(log, in);
		Log::Close(log);
	}
	catch (Error::ERROR e)
	{
		Log::WriteError(log, e);
	}
	return 0;
}


#include "stdafx.h"
#include "Parser.h"
#include "Error.h"
#include "FST.h"

#include <iostream>
#include <locale>


namespace Constants
{
	// Information messages
	const std::string lexemsTableMessage = "----- Таблица лексем -----";
	const std::string identifiersTableMessage = "----- Таблица идентификаторов -----";

	// Tokens
	const std::string integerToken = "integer";
	const std::string stringToken = "string";
	const std::string functionToken = "function";
	const std::string declareToken = "declare";
	const std::string returnToken = "return";
	const std::string printToken = "print";
	const std::string mainToken = "main";
	const std::string semicolonToken = ";";
	const std::string commaToken = ",";
	const std::string leftBraceToken = "{";
	const std::string rightBraceToken = "}";
	const std::string leftBracketToken = "(";
	const std::string rightBracketToken = ")";
	const std::string plusToken = "+"; 
	const std::string minusToken = "-";
	const std::string starToken = "*";
	const std::string slashToken = "/";
	const std::string assignmentToken = "=";
} // namespace Constants

Parser::Parser(Log::LOG log) : m_line(1), m_position(1), m_isFunction(false)
{
	m_log = log;
	m_lexTable = LT::Create(LT_MAXSIZE);
	m_idTable = IT::Create(TI_MAXSIZE);
	m_state = State::Token;
	m_parseWait = ParseWait::Any;
}

void Parser::checkToken()
{
	LT::Entry lexEntry;
	lexEntry.sn = m_line;
	lexEntry.idxTI = LT_TI_NULLIDX;
	if (m_token == Constants::integerToken)
		memcpy(lexEntry.lexema, LEX_INTEGER, LEXEMA_FIXSIZE);
	else if (m_token == Constants::commaToken)
		memcpy(lexEntry.lexema, LEX_COMMA, LEXEMA_FIXSIZE);
	else if (m_token == Constants::declareToken)
		memcpy(lexEntry.lexema, LEX_DECLARE, LEXEMA_FIXSIZE);
	else if (m_token == Constants::functionToken)
		memcpy(lexEntry.lexema, LEX_FUNCTION, LEXEMA_FIXSIZE);
	else if (m_token == Constants::leftBraceToken)
		memcpy(lexEntry.lexema, LEX_LEFTBRACE, LEXEMA_FIXSIZE);
	else if (m_token == Constants::leftBracketToken)
		memcpy(lexEntry.lexema, LEX_LEFTBRACKET, LEXEMA_FIXSIZE);
	else if (m_token == Constants::mainToken)
		memcpy(lexEntry.lexema, LEX_MAIN, LEXEMA_FIXSIZE);
	else if (m_token == Constants::minusToken)
		memcpy(lexEntry.lexema, LEX_MINUS, LEXEMA_FIXSIZE);
	else if (m_token == Constants::plusToken)
		memcpy(lexEntry.lexema, LEX_PLUS, LEXEMA_FIXSIZE);
	else if (m_token == Constants::printToken)
		memcpy(lexEntry.lexema, LEX_PRINT, LEXEMA_FIXSIZE);
	else if (m_token == Constants::returnToken)
		memcpy(lexEntry.lexema, LEX_RETURN, LEXEMA_FIXSIZE);
	else if (m_token == Constants::rightBraceToken)
		memcpy(lexEntry.lexema, LEX_RIGHTBRACE, LEXEMA_FIXSIZE);
	else if (m_token == Constants::rightBracketToken)
		memcpy(lexEntry.lexema, LEX_RIGHTBRACKET, LEXEMA_FIXSIZE);
	else if (m_token == Constants::semicolonToken)
		memcpy(lexEntry.lexema, LEX_SEMICOLON, LEXEMA_FIXSIZE);
	else if (m_token == Constants::slashToken)
		memcpy(lexEntry.lexema, LEX_SLASH, LEXEMA_FIXSIZE);
	else if (m_token == Constants::starToken)
		memcpy(lexEntry.lexema, LEX_STAR, LEXEMA_FIXSIZE);
	else if (m_token == Constants::stringToken)
		memcpy(lexEntry.lexema, LEX_STRING, LEXEMA_FIXSIZE);
	else if (m_token == Constants::assignmentToken)
		memcpy(lexEntry.lexema, LEX_ASSIGNMENT, LEXEMA_FIXSIZE);
	else if (m_state == State::StringLiteral)
	{
		memcpy(lexEntry.lexema, LEX_LITERAL, LEXEMA_FIXSIZE);
	}
	else
	{
		bool allDigits = true;
		for (const char chr : std::as_const(m_token))
		{
			if (!isdigit(chr))
			{
				allDigits = false;
				break;
			}
		}
		if (allDigits)
			memcpy(lexEntry.lexema, LEX_LITERAL, LEXEMA_FIXSIZE);
		else
			memcpy(lexEntry.lexema, LEX_ID, LEXEMA_FIXSIZE);
	}
	LT::Add(m_lexTable, lexEntry);
}

void Parser::commitToken()
{
	checkToken();

	if (m_parseWait == ParseWait::Any)
	{
		if (isTokenType())
		{
			m_parseWait = ParseWait::Function;
			applyTokenType();
		}
		else if (isTokenMainToken())
		{
			m_parseWait = ParseWait::BlockBeginsOrSemicolon;
		}
		else
		{
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
		}
	}
	else if (m_parseWait == ParseWait::Function)
	{
		if (isTokenFunctionToken())
		{
			m_parseWait = ParseWait::IdentifierAfterFunction;
			m_isFunction = true;
		}
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::IdentifierAfterFunction)
	{
		if (isTokenValidIdentifierName())
		{
			IT::Entry entry;
			strncpy_s(entry.id, m_token.c_str(), ID_MAXSIZE);
			entry.idtype = IT::F;
			if (m_type == Type::String)
				entry.iddatatype = IT::STR;
			else if (m_type == Type::Integer)
				entry.iddatatype = IT::INT;
			entry.idxfirstLE = m_lexTable.size - 1;
			IT::Add(m_idTable, entry);

			m_isFunction = false;

			m_parseWait = ParseWait::LeftBracketAfterFunction;
		}
		else
		{
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
		}
	}
	else if (m_parseWait == ParseWait::LeftBracketAfterFunction)
	{
		if (isTokenLeftBracketToken())
			m_parseWait = ParseWait::ParamTypeOrRightBracket;
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::ParamTypeOrRightBracket)
	{
		if (isTokenRightBracketToken())
		{
			m_parseWait = ParseWait::BlockBeginsOrSemicolon;
		}
		else if (isTokenType())
		{
			m_parseWait = ParseWait::ParamName;
			applyTokenType();
		}
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::ParamName)
	{
		if (isTokenValidIdentifierName())
		{
			IT::Entry entry;
			strncpy_s(entry.id, m_token.c_str(), ID_MAXSIZE);
			entry.idtype = IT::P;
			if (m_type == Type::String)
				entry.iddatatype = IT::STR;
			else if (m_type == Type::Integer)
				entry.iddatatype = IT::INT;
			entry.idxfirstLE = m_lexTable.size - 1;
			IT::Add(m_idTable, entry);

			m_isFunction = false;

			m_parseWait = ParseWait::ParamCommaOrRightBracket;
		}
		else
		{
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
		}
	}
	else if (m_parseWait == ParseWait::ParamCommaOrRightBracket)
	{
		if (isTokenCommaToken())
			m_parseWait = ParseWait::ParamType;
		else if (isTokenRightBracketToken())
			m_parseWait = ParseWait::BlockBeginsOrSemicolon;
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::ParamType)
	{
		if (isTokenType())
		{
			m_parseWait = ParseWait::ParamName;
			applyTokenType();
		}
		else
		{
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
		}
	}
	else if (m_parseWait == ParseWait::BlockBeginsOrSemicolon)
	{
		if (isTokenSemicolonToken())
			m_parseWait = ParseWait::Any;
		else if (isTokenLeftBraceToken())
			m_parseWait = ParseWait::InBlockAny;
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::InBlockAny)
	{
		if (isTokenDeclareToken())
			m_parseWait = ParseWait::InBlockType;
		else if (isTokenValidIdentifierName())
			m_parseWait = ParseWait::InBlockAssignment;
		else if (isTokenReturnToken())
			m_parseWait = ParseWait::InBlockIdentifierOrLiteral;
		else if (isTokenRightBraceToken())
			m_parseWait = ParseWait::EndBlockSemicolon;
		else if (isTokenPrintToken())
			m_parseWait = ParseWait::InBlockExpression;
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::InBlockType)
	{
		if (isTokenType())
		{
			m_parseWait = ParseWait::InBlockIdentifierOrFunctionOrExpression;
			applyTokenType();
		}
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::InBlockIdentifierOrFunctionOrExpression)
	{
		if (isTokenValidIdentifierName())
		{
			IT::Entry entry;
			strncpy_s(entry.id, m_token.c_str(), ID_MAXSIZE);
			entry.idtype = IT::V;
			if (m_type == Type::String)
				entry.iddatatype = IT::STR;
			else if (m_type == Type::Integer)
				entry.iddatatype = IT::INT;
			entry.idxfirstLE = m_lexTable.size - 1;
			IT::Add(m_idTable, entry);

			m_isFunction = false;

			m_parseWait = ParseWait::InBlockSemicolon;
		}
		else if (isTokenFunctionToken())
			m_parseWait = ParseWait::InBlockExpression;
		else
			m_parseWait = ParseWait::InBlockExpression;
	}
	else if (m_parseWait == ParseWait::InBlockSemicolon)
	{
		if (isTokenSemicolonToken())
			m_parseWait = ParseWait::InBlockAny;
		else
			m_parseWait = ParseWait::InBlockExpression;
	}
	else if (m_parseWait == ParseWait::InBlockAssignment)
	{
		if (isTokenAssignmentToken())
			m_parseWait = ParseWait::InBlockExpression;
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::InBlockExpression)
	{
		if (isTokenSemicolonToken())
			m_parseWait = ParseWait::InBlockAny;
		else
			m_parseWait = ParseWait::InBlockExpression;
	}
	else if (m_parseWait == ParseWait::InBlockIdentifierOrLiteral)
	{
		if (isTokenValidIdentifierName() || isTokenNumberLiteral() || isTokenStringLiteral())
			m_parseWait = ParseWait::InBlockSemicolon;
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else if (m_parseWait == ParseWait::EndBlockSemicolon)
	{
		if (isTokenSemicolonToken())
			m_parseWait = ParseWait::Any;
		else
			throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());
	}
	else
		throw ERROR_THROW_IN(3, m_line, m_position - m_token.size());

	m_token.clear();
}

void Parser::applyTokenType()
{
	if (m_token == Constants::stringToken)
		m_type = Type::String;
	else if (m_token == Constants::integerToken)
		m_type = Type::Integer;
}

bool Parser::isTokenType()
{
	if (   m_token == Constants::stringToken
		|| m_token == Constants::integerToken)
		return true;
	return false;
}

bool Parser::isTokenPredefinedWord()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::declareToken)
		return true;
	if (m_token == Constants::functionToken)
		return true;
	if (m_token == Constants::integerToken)
		return true;
	if (m_token == Constants::mainToken)
		return true;
	if (m_token == Constants::printToken)
		return true;
	if (m_token == Constants::returnToken)
		return true;
	if (m_token == Constants::stringToken)
		return true;
	return false;
}

bool Parser::isTokenStringLiteral()
{
	if (m_state == State::StringLiteral)
		return true;
	return false;
}

bool Parser::isTokenNumberLiteral()
{
	if (m_token.empty())
		return false;
	for (size_t i = 0; i < m_token.size(); ++i)
	{
		if (!isdigit(m_token[i]))
			return false;
	}
	return true;
}

bool Parser::isTokenValidIdentifierName()
{
	if (m_token.empty())
		return false;
	if (isTokenStringLiteral())
		return false;
	if (isTokenPredefinedWord())
		return false;
	for (size_t i = 0; i < m_token.size(); ++i)
	{
		if (!islower(m_token[i]))
			return false;
	}
	return true;
}

bool Parser::isTokenFunctionToken()
{
	if (m_state == State::StringLiteral)
		return false;

	FST::FST fst(m_token.c_str(), 9
		,	FST::NODE(1, FST::RELATION('f', 1))	//	0
		,	FST::NODE(1, FST::RELATION('u', 2))	//	1
		,	FST::NODE(1, FST::RELATION('n', 3))	//	2
		,	FST::NODE(1, FST::RELATION('c', 4))	//	3
		,	FST::NODE(1, FST::RELATION('t', 5))	//	4
		,	FST::NODE(1, FST::RELATION('i', 6))	//	5
		,	FST::NODE(1, FST::RELATION('o', 7))	//	6
		,	FST::NODE(1, FST::RELATION('n', 8))	//	7
		,	FST::NODE()			//	8
		);

	if (FST::execute(fst))
		return true;
	return false;
}

bool Parser::isTokenLeftBracketToken()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::leftBracketToken)
		return true;
	return false;
}

bool Parser::isTokenRightBracketToken()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::rightBracketToken)
		return true;
	return false;
}

bool Parser::isTokenLeftBraceToken()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::leftBraceToken)
		return true;
	return false;
}

bool Parser::isTokenRightBraceToken()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::rightBraceToken)
		return true;
	return false;
}

bool Parser::isTokenSemicolonToken()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::semicolonToken)
		return true;
	return false;
}

bool Parser::isTokenCommaToken()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::commaToken)
		return true;
	return false;
}

bool Parser::isTokenDeclareToken()
{
	FST::FST fst(m_token.c_str(), 8
		, FST::NODE(1, FST::RELATION('d', 1))	//	0
		, FST::NODE(1, FST::RELATION('e', 2))	//	1
		, FST::NODE(1, FST::RELATION('c', 3))	//	2
		, FST::NODE(1, FST::RELATION('l', 4))	//	3
		, FST::NODE(1, FST::RELATION('a', 5))	//	4
		, FST::NODE(1, FST::RELATION('r', 6))	//	5
		, FST::NODE(1, FST::RELATION('e', 7))	//	6
		, FST::NODE()	//	7
	);
	if (FST::execute(fst))
		return true;
	return false;
}

bool Parser::isTokenAssignmentToken()
{
	if (m_state == State::StringLiteral)
		return false;
	if (m_token == Constants::assignmentToken)
		return true;
	return false;
}

bool Parser::isTokenOperator()
{
	if (m_state == State::StringLiteral)
		return false;
	if  (   m_token == Constants::plusToken
		||	m_token == Constants::minusToken
		||	m_token == Constants::starToken
		||	m_token == Constants::slashToken )
		return true;
	return false;
}

bool Parser::isTokenReturnToken()
{
	if (m_state == State::StringLiteral)
		return false;

	FST::FST fst(m_token.c_str(), 7
		, FST::NODE(1, FST::RELATION('r', 1))	//	0
		, FST::NODE(1, FST::RELATION('e', 2))	//	1
		, FST::NODE(1, FST::RELATION('t', 3))	//	2
		, FST::NODE(1, FST::RELATION('u', 4))	//	3
		, FST::NODE(1, FST::RELATION('r', 5))	//	4
		, FST::NODE(1, FST::RELATION('n', 6))	//	5
		, FST::NODE()	//	6
	);
	if (FST::execute(fst))
		return true;

	return false;
}

bool Parser::isTokenMainToken()
{
	if (m_state == State::StringLiteral)
		return false;

	FST::FST fst(m_token.c_str(), 5
		,	FST::NODE(1, FST::RELATION('m', 1))	//	0
		,	FST::NODE(1, FST::RELATION('a', 2))	//	1
		,	FST::NODE(1, FST::RELATION('i', 3)) //	2
		,	FST::NODE(1, FST::RELATION('n', 4))	//	3
		,	FST::NODE()	//	4
		);
	if (FST::execute(fst))
		return true;
	return false;
}

bool Parser::isTokenPrintToken()
{
	if (m_state == State::StringLiteral)
		return false;

	FST::FST fst(m_token.c_str(), 6
		,	FST::NODE(1, FST::RELATION('p', 1))	//	0
		,	FST::NODE(1, FST::RELATION('r', 2))	//	1
		,	FST::NODE(1, FST::RELATION('i', 3)) //	2
		,	FST::NODE(1, FST::RELATION('n', 4))	//	3
		,	FST::NODE(1, FST::RELATION('t', 5))	//	4
		,	FST::NODE()	//	5
	);

	if (FST::execute(fst))
		return true;
	return false;
}

std::string Parser::intToStr(int par, int precision)
{
	char data[128];
	_itoa_s(par, data, 10);
	std::string result(data);
	int pr = precision - result.size();
	if (pr > 0)
	{
		for (int i = 0; i < pr; ++i)
			result.insert(result.begin(), '0');
	}
	return result;
}

void Parser::putChar(const unsigned char ch)
{
	if (m_state == State::StringLiteral)
	{
		if (ch == '\'')
		{
			commitToken();
			m_state = State::Token;
		}
		return;
	}

	if (isspace(ch))
	{
		if (m_token.empty())
		{

		}
		else
		{
			commitToken();
		}
	}
	else 
	{
		if (m_token.empty())
		{
			if (ch == '\'')
			{
				m_state = State::StringLiteral;
			}
			else
			{
				m_token += ch;
			}
		}
		else
		{
			unsigned char lastTokenChar = static_cast<unsigned char>(m_token[m_token.size() - 1]);
			if (islower( lastTokenChar ))
			{
				if (islower(ch))
				{
					m_token += ch;
				}
				else if (!isdigit(ch))
				{
					commitToken();
					m_token += ch;
				}
				else
					throw ERROR_THROW_IN(2, m_line, m_position);
			}
			else if (isdigit(lastTokenChar))
			{
				if (isdigit(ch))
				{
					m_token += ch;
				}
				else if (!isalpha(ch))
				{
					commitToken();
					m_token += ch;
				}
				else
					throw ERROR_THROW_IN(2, m_line, m_position);
			}
			else
			{
				commitToken();
				m_token += ch;
			}
		}
	}
	++m_position;
	if (ch == '\n')
	{
		m_position = 1;
		++m_line;
	}
}

void Parser::printLexems()
{
	Log::WriteLine(m_log, Constants::lexemsTableMessage.c_str(), "");
	int currentLineIdx = 1;
	std::string currentLine;
	for (int i = 0; i < m_lexTable.size;)
	{
		char currentLexema[LEXEMA_FIXSIZE + 1];
		strncpy_s(currentLexema, m_lexTable.table[i].lexema, LEXEMA_FIXSIZE);
		if (currentLineIdx == m_lexTable.table[i].sn)
		{
			currentLine += currentLexema;
		}
		else
		{
			Log::WriteLine(m_log, intToStr(currentLineIdx).c_str(), " ", currentLine.c_str(), "");
			currentLine.clear();
			++currentLineIdx;
			continue;
		}
		++i;
	}
	Log::WriteLine(m_log, intToStr(currentLineIdx).c_str(), " ", currentLine.c_str(), "");
}

void Parser::printIdentifiers()
{
	Log::WriteLine(m_log, Constants::identifiersTableMessage.c_str(), "");
	for (int i = 0; i < m_idTable.size; ++i)
	{
		Log::WriteLine(m_log,
			m_idTable.table[i].idtype == IT::F ? "function" : m_idTable.table[i].idtype == IT::P ? "param" : "variable", "\t",
			m_idTable.table[i].iddatatype == IT::STR ? "string" : "integer", "\t",
			m_idTable.table[i].id, "\t", intToStr(m_idTable.table[i].idxfirstLE).c_str(), "");
	}
}

Parser::~Parser()
{
	LT::Delete(m_lexTable);
	IT::Delete(m_idTable);
}
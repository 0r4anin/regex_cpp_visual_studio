#include "pch.h"
#include "Pattern.h"

PatternBase::PatternBase(PatternType _type) : type(_type)
{
}

bool PatternBase::AllreadyMatched() const
{
	return matched;
}

PatternSymbol::PatternSymbol(char symbol)
	: PatternBase(PatternBase::PSymbol)
	, m_symbol(symbol)
{

}

bool PatternSymbol::Match(char c)
{
	if (c == m_symbol)
	{
		matched = true;
		return true;
	}

	return false;
}

PatternQuestion::PatternQuestion() : PatternBase(PatternBase::PQuestion)
{
}

bool PatternQuestion::Match(char c)
{
	matched = true;
	return true;
}

PatternSkip::PatternSkip() 
	: PatternBase(PatternBase::PSkip)
{
	matched = true;
}

bool PatternSkip::Match(char c)
{
	matched = true;
	return true;
}

PatternSpecSymbol::PatternSpecSymbol(PatternBase::PatternType type)
	: PatternBase(type)
{
}

bool PatternSpecSymbol::Match(char c)
{
	switch (type)
	{
	case PatternBase::PNum:
		if (c >= '0' && c <= '9')
		{
			matched = true;
			return true;
		}
		break;
	case PatternBase::PAlpha:
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		{
			matched = true;
			return true;
		}
		break;
	case PatternBase::PAlphaNum:
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')
		{
			matched = true;
			return true;
		}
		break;
	default:
		return false;
	}
	
	return false;
}

PatternSet::PatternSet()
	: PatternBase(PatternBase::PSet)
	, m_pairCount(0)
	, m_symbolCount(0)
{
	
}

PatternSet::~PatternSet()
{
	if (m_pairCount > 0)
		delete[] m_arrayPair;
	if (m_symbolCount > 0)
		delete[] m_arraySymbol;
}

bool PatternSet::Match(char c)
{
	for (int i = 0; i < m_pairCount; ++i)
	{
		if (c >= m_arrayPair[i].from && c <= m_arrayPair[i].to)
			return true;
	}

	for (int i = 0; i < m_symbolCount; ++i)
	{
		if (c == m_arraySymbol[i])
			return true;
	}

	return false;
}

bool PatternSet::parseSet(const char *start, const char *end)
{
	if (start >= end)
	{
		return false;
	}

	//count elements
	for (const char *cur = start; cur < end; ++cur)
	{
		if ((cur + 2) < end && *(cur + 1) == '-')
		{
			++m_pairCount;
			cur += 2;
		}
		else
		{
			++m_symbolCount;
		}
	}

	if (m_pairCount)
	{
		m_arrayPair = new SetPair[m_pairCount];
	}

	if (m_symbolCount)
	{
		m_arraySymbol = new char[m_symbolCount];
	}

	int i = 0, j = 0;
	for (const char* cur = start; cur < end; ++cur)
	{
		if (((cur + 2) < end) && (cur[1] == '-'))
		{
			m_arrayPair[i].from = *cur;
			cur += 2;
			m_arrayPair[i].to = *cur;
			
			if (m_arrayPair[i].from >= m_arrayPair[i].to)
				return false;

			++i;
		}
		else
		{
			m_arraySymbol[j++] = *cur;
		}
	}

	return true;
}

PatternRange::PatternRange(int min, int max, PatternBase * parent) 
	: PatternBase(PatternBase::PRange)
	, m_min(min)
	, m_max(max)
	, m_cur(0)
	, m_parent(parent)
{
}

PatternRange::~PatternRange() 
{

	if (m_parent)
		delete m_parent;
}

bool PatternRange::Match(char c)
{
	if (m_parent == nullptr || (m_min > m_max && m_max !=0))
		return false;

	if (m_parent->Match(c))
	{
		++m_cur;
		return m_max == 0 || m_cur <= m_max;
	}

	return false;
}

bool PatternRange::CanSkip() const
{
	return m_cur >= m_min && (m_max == 0 || m_cur < m_max);
}

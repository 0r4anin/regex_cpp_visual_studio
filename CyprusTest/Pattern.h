#pragma once

class PatternBase
{
public:
	enum PatternType
	{
		PUnknown = 0,
		PSymbol,
		PQuestion, 
		PSkip, 
		PNum,
		PAlpha,
		PAlphaNum, 
		PSet,
		PRange
	};
protected:
	PatternBase(PatternType _type);
public:
	virtual ~PatternBase() {}

	virtual bool Match(char c) = 0;
	virtual void Reset()
	{
		matched = false;
	}

	virtual bool AllreadyMatched() const;

	virtual bool CanSkip() const
	{
		return false;
	}

	virtual void Decrement()
	{
		matched = false;
	}

	const PatternType type;

protected:
	bool matched;
};

class PatternSymbol : public PatternBase
{
public:
	PatternSymbol(char symbol);
	~PatternSymbol() {}

	virtual bool Match(char c);
private:
	char m_symbol;
};

class PatternQuestion : public PatternBase
{
public:
	PatternQuestion();
	~PatternQuestion() {}

	virtual bool Match(char c);

};

class PatternSkip : public PatternBase
{
public:
	PatternSkip();
	~PatternSkip() {}

	virtual bool Match(char c);
	virtual bool CanSkip() const
	{
		return true;
	}
	virtual void Reset()
	{
		matched = true;
	}
	virtual void Decrement()
	{
		matched = false;
	}

};

class PatternSpecSymbol : public PatternBase
{
public:
	PatternSpecSymbol(PatternBase::PatternType type);
	~PatternSpecSymbol() {}

	virtual bool Match(char c);
};

class PatternSet : public PatternBase
{
private:
	struct SetPair
	{
		char from, to;
	};
public:
	PatternSet();
	~PatternSet();

	virtual bool Match(char c);

	bool parseSet(const char *start, const char *end);

private:
	SetPair *m_arrayPair;
	int m_pairCount;
	char *m_arraySymbol;
	int m_symbolCount;
};

class PatternRange : public PatternBase
{
public:
	PatternRange(int min, int max, PatternBase *parent);
	~PatternRange();

	virtual bool Match(char c);

	virtual bool CanSkip()const;
	virtual bool AllreadyMatched() const
	{
		return CanSkip();
	}
	virtual void Decrement()
	{
		--m_cur;
	}

	virtual void Reset()
	{
		m_cur = 0;
	}
private:
	int m_min;
	int m_max;
	int m_cur;
	PatternBase *m_parent;
};
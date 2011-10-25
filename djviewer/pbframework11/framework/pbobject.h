#ifndef PBOBJECT_H
#define PBOBJECT_H

#include <assert.h>
#include <stdio.h>

class _PBObjectChecker
{
public:
	_PBObjectChecker()
	{
	};

	virtual ~_PBObjectChecker()
	{
		assert(m_checkCounter == 0);
	};

	static void Increment()
	{
		// todo - use thread synchronization
		m_checkCounter++;
		//fprintf (stderr, "inc - %d\n", m_checkCounter);
	};

	static void Decrement()
	{
		m_checkCounter--;
		//fprintf (stderr, "dec - %d\n", m_checkCounter);
	};

protected:
	static int m_checkCounter;
};

class PBObject
{
public:
	PBObject()
		:m_refCount(0)
	{
#ifdef EMULATOR
		_PBObjectChecker::Increment();
#endif
	};

	virtual ~PBObject()
	{
#ifdef EMULATOR
		_PBObjectChecker::Decrement();
#endif
	};

	int AddRef()
	{
		return ++m_refCount;
	};

	int Release()
	{
		m_refCount--;
		assert(m_refCount >= 0);

		if (m_refCount == 0)
			delete this;

		return m_refCount;
	};

protected:
	int m_refCount;
};


#endif // PBOBJECT_H

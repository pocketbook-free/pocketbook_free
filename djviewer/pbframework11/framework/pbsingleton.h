#ifndef PBSINGLETON_H
#define PBSINGLETON_H

#include "pbobject.h"

template <class T>
class PBSingleton : public PBObject
{
	static T* s_pThis;

protected:
	PBSingleton()
	{
	};

	virtual ~PBSingleton()
	{
		s_pThis=0;
	};

	virtual bool Init() = 0;

public:
	static T* Instance();
	static T* GetThis();
};

template <class T>
T*  PBSingleton<T>::s_pThis = 0;


template <class T>
T*  PBSingleton<T>::Instance()
{
	if(!s_pThis)
	{
		s_pThis = new T;
		s_pThis->Init();
	}
	return s_pThis;
}

template <class T>
T*  PBSingleton<T>::GetThis()
{
	return s_pThis;
}

#endif // PBSINGLETON_H

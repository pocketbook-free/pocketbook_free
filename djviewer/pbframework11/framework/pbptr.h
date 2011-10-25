#ifndef PBPTR_H
#define PBPTR_H


template<class RC> class PBPtr
{
private:
	RC * m_ptr;


public:

	// Default constructor - creates null pointer.
	PBPtr()
		: m_ptr(0)
	{
	}

	//	Create a smart pointer from the raw pointer, incrementing reference count.
	PBPtr( RC * ptr )
		:m_ptr(ptr)
	{
		if(ptr)
			ptr->AddRef();
	}

	//	Create a smart pointer from another smart pointer, incrementing reference count.
	PBPtr( const PBPtr<RC>& other ) : m_ptr(other.m_ptr)
	{
		if(m_ptr)
			m_ptr->AddRef();
	}

	//	Destructor - decremented reference count.
	~PBPtr()
	{
		if(m_ptr)
			m_ptr->Release();
	}

	void Release()
	{
		if(m_ptr)
			m_ptr->Release();
		m_ptr = NULL;
	}

	//	Obtain raw pointer from smart pointer. Reference count is not touched.
	operator RC*() const
	{
		return m_ptr;
	}

	//	Return false for null pointer, true otherwise.
	operator bool() const
	{
		return m_ptr != 0;
	}

	//	Obtain raw pointer from smart pointer for member access. Reference count is not touched.
	RC* operator->() const
	{
		return m_ptr;
	}

	//	Assignment operator. Reference count of the assigned pointer is incremented.
	//	Then, reference count of this object is decremented.
	const PBPtr<RC>& operator=( const PBPtr<RC>& other )
	{
		if( other.m_ptr )
			other.m_ptr->AddRef();
		if( m_ptr )
			m_ptr->Release();
		m_ptr = other.m_ptr;
		return *this;
	}
};

#endif // PBPTR_H

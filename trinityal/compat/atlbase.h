#pragma once

// Used only when the MSVC ATL headers are not installed.
// CComPtr and CComQIPtr cover the COM lifetime calls in TrinityAL.

#include <cstddef>

namespace ATL
{

template <typename T>
class CComPtr
{
public:
	T* p;

	CComPtr()
		: p( nullptr )
	{
	}

	CComPtr( T* source )
		: p( source )
	{
		if( p )
			p->AddRef();
	}

	CComPtr( const CComPtr& other )
		: p( other.p )
	{
		if( p )
			p->AddRef();
	}

	~CComPtr()
	{
		Release();
	}

	void Release()
	{
		T* current = p;
		if( current )
		{
			p = nullptr;
			current->Release();
		}
	}

	void Attach( T* source )
	{
		Release();
		p = source;
	}

	T* Detach()
	{
		T* current = p;
		p = nullptr;
		return current;
	}

	operator T*() const
	{
		return p;
	}

	T& operator*() const
	{
		return *p;
	}

	T* operator->() const
	{
		return p;
	}

	T** operator&()
	{
		return &p;
	}

	bool operator!() const
	{
		return p == nullptr;
	}

	T* operator=( T* source )
	{
		if( p == source )
			return p;
		if( source )
			source->AddRef();
		Release();
		p = source;
		return p;
	}

	T* operator=( const CComPtr& other )
	{
		return operator=( other.p );
	}

	template <typename Q>
	HRESULT QueryInterface( Q** out ) const
	{
		return p->QueryInterface( __uuidof( Q ), reinterpret_cast<void**>( out ) );
	}
};

template <typename T>
class CComQIPtr : public CComPtr<T>
{
public:
	CComQIPtr()
	{
	}

	template <typename Q>
	explicit CComQIPtr( Q* source )
	{
		Adopt( source );
	}

	template <typename Q>
	explicit CComQIPtr( const CComPtr<Q>& source )
	{
		Adopt( source.p );
	}

	template <typename Q>
	T* operator=( Q* source )
	{
		Adopt( source );
		return this->p;
	}

private:
	template <typename Q>
	void Adopt( Q* source )
	{
		this->Release();
		if( source )
			source->QueryInterface( __uuidof( T ), reinterpret_cast<void**>( &this->p ) );
	}
};

}

using ATL::CComPtr;
using ATL::CComQIPtr;

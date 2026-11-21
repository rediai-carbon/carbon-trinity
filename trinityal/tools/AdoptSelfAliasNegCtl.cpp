#include <cstdio>
#include <Windows.h>
#include <unknwn.h>

#include "compat/atlbase.h"

namespace
{
bool g_useAfterFree = false;
bool g_countReachedZero = false;

struct Probe : public IUnknown
{
	unsigned long refs = 1;

	STDMETHODIMP QueryInterface( REFIID riid, void** out ) override
	{
		if( refs == 0 )
			g_useAfterFree = true;
		if( riid == IID_IUnknown )
		{
			*out = this;
			AddRef();
			return S_OK;
		}
		*out = nullptr;
		return E_NOINTERFACE;
	}

	STDMETHODIMP_( ULONG )
	AddRef() override
	{
		return ++refs;
	}

	STDMETHODIMP_( ULONG )
	Release() override
	{
		const ULONG next = --refs;
		if( next == 0 )
			g_countReachedZero = true;
		return next;
	}
};
}

int main()
{
	Probe* obj = new Probe();
	CComQIPtr<IUnknown> qi;
	qi.p = obj;
	IUnknown* self = qi.p;
	qi = self;

	if( g_useAfterFree )
	{
		std::printf( "ADOPT_UAF_DETECTED zero_count_before_query=%d\n", g_countReachedZero ? 1 : 0 );
		return 1;
	}
	if( qi.p != self )
	{
		std::printf( "ADOPT_WRONG_POINTER\n" );
		return 1;
	}
	std::printf( "ADOPT_SELF_ALIAS_OK refs=%lu\n", obj->refs );
	return 0;
}
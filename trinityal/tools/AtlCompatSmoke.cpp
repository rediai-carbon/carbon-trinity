#include <cstdio>
#include <Windows.h>

#if __has_include( <atlbase.h> )
#include <atlbase.h>
#define TRINITY_AL_USE_SYSTEM_ATL 1
#else
#include "compat/atlbase.h"
#define TRINITY_AL_USE_SYSTEM_ATL 0
#endif

namespace
{
struct FakeUnknown : public IUnknown
{
	unsigned long refs = 1;
	STDMETHODIMP QueryInterface( REFIID riid, void** out ) override
	{
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
		const auto next = --refs;
		if( next == 0 )
			delete this;
		return next;
	}
};

bool Expect( bool cond, const char* step )
{
	std::printf( "%s %s\n", step, cond ? "OK" : "FAIL" );
	return cond;
}
}

int main()
{
	std::printf( "TRINITY_AL_USE_SYSTEM_ATL=%d\n", TRINITY_AL_USE_SYSTEM_ATL );

	FakeUnknown* raw = new FakeUnknown();
	{
		CComPtr<FakeUnknown> a;
		a.Attach( raw );
		if( !Expect( a.p == raw && raw->refs == 1, "Attach" ) )
			return 1;
		{
			CComPtr<FakeUnknown> b = a;
			if( !Expect( raw->refs == 2, "CopyAddRef" ) )
				return 1;
		}
		if( !Expect( raw->refs == 1, "ReleaseCopy" ) )
			return 1;
		IUnknown* q = nullptr;
		if( !Expect( SUCCEEDED( a.QueryInterface( &q ) ), "QI" ) )
			return 1;
		q->Release();
		FakeUnknown* detached = a.Detach();
		if( !Expect( detached == raw && a.p == nullptr, "Detach" ) )
			return 1;
		raw = detached;
	}
	if( !Expect( raw->refs == 1, "ScopeEnd" ) )
		return 1;
	raw->Release();
	if( !Expect( true, "Destroyed" ) )
		return 1;

	{
		FakeUnknown* raw2 = new FakeUnknown();
		CComPtr<FakeUnknown> holder;
		holder.Attach( raw2 );
		CComQIPtr<IUnknown> qi( holder );
		if( !Expect( qi.p != nullptr, "QIPtr_IUnknown" ) )
			return 1;
		IUnknown* self = qi.p;
		qi = self;
		if( !Expect( qi.p == self && raw2->refs >= 1, "QIPtr_self_alias" ) )
			return 1;
		holder.Release();
	}

	std::printf( "ATL_COMPAT_OK\n" );
	return 0;
}
#include <cstdint>
#include <cstdio>

const char* g_moduleName = "dx11-warp-smoke";

#include "StdAfx.h"
#include "include/TrinityAL.h"

namespace
{
bool Ok( const char* step, ALResult result )
{
	if( FAILED( result ) )
	{
		const HRESULT code = result;
		std::printf( "%s FAIL 0x%08lx\n", step, static_cast<unsigned long>( code ) );
		return false;
	}
	std::printf( "%s OK\n", step );
	return true;
}
}

int main()
{
	Tr2PrimaryRenderContextAL context;
	Tr2PrimaryRenderContextAL::SetPrimaryRenderContext( &context );

	Tr2PresentParametersAL parameters = {};
	parameters.mode.width = 64;
	parameters.mode.height = 64;
	parameters.mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM;
	parameters.backBufferCount = 1;
	parameters.swapEffect = Tr2RenderContextEnum::SWAP_EFFECT_DISCARD;
	parameters.windowed = true;
	parameters.software = true;
	parameters.presentInterval = Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE;

	if( !Ok( "CreateDevice", context.CreateDevice( 0, 0, parameters ) ) )
		return 1;

	const Tr2BitmapDimensions size( 4, 4, 1, ImageIO::PIXEL_FORMAT_B8G8R8A8_UNORM );
	Tr2TextureAL target;
	if( !Ok( "CreateTarget", target.Create( size, Tr2GpuUsage::RENDER_TARGET, context ) ) )
		return 1;
	if( !Ok( "SetTarget", context.SetRenderTarget( target ) ) )
		return 1;

	const uint32_t green = 0xff00ff00;
	if( !Ok( "Clear", context.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, green, 1.0f ) ) )
		return 1;

	Tr2TextureAL readback;
	if( !Ok( "CreateReadback", readback.Create( size, Tr2GpuUsage::COPY_DESTINATION, Tr2CpuUsage::READ, context ) ) )
		return 1;

	Tr2TextureSubresource region( 0u );
	if( !Ok( "Copy", readback.CopySubresourceRegion( region, target, region, context ) ) )
		return 1;

	const void* data = nullptr;
	uint32_t pitch = 0;
	if( !Ok( "Map", readback.MapForReading( region, data, pitch, context ) ) )
		return 1;

	const auto pixel = *static_cast<const uint32_t*>( data );
	std::printf( "PIXEL 0x%08x PITCH %u\n", pixel, pitch );
	if( pixel != green )
	{
		std::printf( "PIXEL_MISMATCH\n" );
		return 1;
	}
	std::printf( "TRINITY_DX11_WARP_OK\n" );
	return 0;
}

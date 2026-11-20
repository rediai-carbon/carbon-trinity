#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

const char* g_moduleName = "dx11-warp-triangle-smoke";

#include "StdAfx.h"
#include "include/TrinityAL.h"

#include <d3dcompiler.h>
#pragma comment( lib, "d3dcompiler.lib" )

namespace
{
constexpr uint32_t kClearGreen = 0xff00ff00;
constexpr uint32_t kRedBgra = 0xffff0000;

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

bool CompileHlsl( const char* source, const char* entry, const char* target, std::vector<uint8_t>& out )
{
	ID3DBlob* code = nullptr;
	ID3DBlob* errors = nullptr;
	const HRESULT hr = D3DCompile(
		source,
		strlen( source ),
		"Dx11WarpTriangleSmoke",
		nullptr,
		nullptr,
		entry,
		target,
		D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&code,
		&errors );
	if( errors )
	{
		std::fprintf( stderr, "%s\n", static_cast<const char*>( errors->GetBufferPointer() ) );
		errors->Release();
	}
	if( FAILED( hr ) || code == nullptr )
	{
		std::printf( "D3DCompile FAIL 0x%08lx\n", static_cast<unsigned long>( hr ) );
		return false;
	}
	out.assign(
		static_cast<const uint8_t*>( code->GetBufferPointer() ),
		static_cast<const uint8_t*>( code->GetBufferPointer() ) + code->GetBufferSize() );
	code->Release();
	return true;
}

uint32_t ReadPixel( Tr2TextureAL& readback, uint32_t x, uint32_t y, Tr2PrimaryRenderContextAL& context, uint32_t width, uint32_t height )
{
	Tr2TextureSubresource region( 0u );
	const void* data = nullptr;
	uint32_t pitch = 0;
	if( FAILED( readback.MapForReading( region, data, pitch, context ) ) )
	{
		return 0;
	}
	const auto* row = static_cast<const uint8_t*>( data ) + y * pitch;
	const auto pixel = *reinterpret_cast<const uint32_t*>( row + x * 4 );
	readback.UnmapForReading( context );
	return pixel;
}
} // namespace

int main()
{
	static const char kVsHlsl[] =
		"struct VS_INPUT { float3 Position : POSITION; };\n"
		"struct VS_OUTPUT { float4 Position : SV_Position; };\n"
		"VS_OUTPUT main(VS_INPUT input) {\n"
		"  VS_OUTPUT o;\n"
		"  o.Position = float4(input.Position, 1);\n"
		"  return o;\n"
		"}\n";
	static const char kPsHlsl[] =
		"struct VS_OUTPUT { float4 Position : SV_Position; };\n"
		"float4 main(VS_OUTPUT In) : SV_Target { return float4(1, 0, 0, 1); }\n";

	std::vector<uint8_t> vsBlob;
	std::vector<uint8_t> psBlob;
	if( !CompileHlsl( kVsHlsl, "main", "vs_4_0", vsBlob ) )
		return 1;
	if( !CompileHlsl( kPsHlsl, "main", "ps_4_0", psBlob ) )
		return 1;

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

	const uint32_t width = 64;
	const uint32_t height = 64;
	const Tr2BitmapDimensions size( width, height, 1, ImageIO::PIXEL_FORMAT_B8G8R8A8_UNORM );
	Tr2TextureAL target;
	if( !Ok( "CreateTarget", target.Create( size, Tr2GpuUsage::RENDER_TARGET, context ) ) )
		return 1;
	if( !Ok( "SetTarget", context.SetRenderTarget( target ) ) )
		return 1;

	Tr2Viewport viewport( width, height );
	if( !Ok( "SetViewport", context.SetViewport( viewport ) ) )
		return 1;

	Tr2ShaderBytecodeAL vsBytecode( vsBlob.data(), vsBlob.size() );
	Tr2ShaderBytecodeAL psBytecode( psBlob.data(), psBlob.size() );

	Tr2ShaderAL vs;
	auto vsSig = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 );
	if( !Ok( "CreateVS", vs.Create( Tr2RenderContextEnum::VERTEX_SHADER, vsBytecode, vsSig, "", context ) ) )
		return 1;

	Tr2ShaderAL ps;
	if( !Ok( "CreatePS", ps.Create( Tr2RenderContextEnum::PIXEL_SHADER, psBytecode, Tr2ShaderSignatureAL(), "", context ) ) )
		return 1;

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL program;
	if( !Ok( "CreateProgram", program.Create( shaders, 2, context ) ) )
		return 1;

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	Tr2VertexLayoutAL layout;
	if( !Ok( "CreateLayout", layout.Create( definition, context ) ) )
		return 1;

	if( !Ok( "BeginScene", context.BeginScene() ) )
		return 1;
	if( !Ok( "Clear", context.Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, kClearGreen, 1.0f ) ) )
		return 1;
	if( !Ok( "SetLayout", context.SetVertexLayout( layout ) ) )
		return 1;
	if( !Ok( "SetProgram", context.SetShaderProgram( program ) ) )
		return 1;
	if( !Ok( "SetTopology", context.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) ) )
		return 1;
	if( !Ok( "SetZOff", context.SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) ) )
		return 1;
	if( !Ok( "SetCullNone", context.SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) ) )
		return 1;

	const float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	if( !Ok( "DrawUP", context.DrawPrimitiveUP( 1, vertices, sizeof( float ) * 3 ) ) )
		return 1;
	if( !Ok( "EndScene", context.EndScene() ) )
		return 1;

	Tr2TextureAL readback;
	if( !Ok( "CreateReadback", readback.Create( size, Tr2GpuUsage::COPY_DESTINATION, Tr2CpuUsage::READ, context ) ) )
		return 1;
	Tr2TextureSubresource region( 0u );
	if( !Ok( "Copy", readback.CopySubresourceRegion( region, target, region, context ) ) )
		return 1;

	const uint32_t corner = ReadPixel( readback, width - 1, 0, context, width, height );
	const uint32_t center = ReadPixel( readback, width / 2, height / 2, context, width, height );
	std::printf( "CORNER 0x%08x CENTER 0x%08x\n", corner, center );

	if( corner != kClearGreen )
	{
		std::printf( "CORNER_NOT_CLEAR\n" );
		return 1;
	}
	if( center != kRedBgra )
	{
		std::printf( "CENTER_NOT_RED expected 0x%08x\n", kRedBgra );
		return 1;
	}
	std::printf( "TRINITY_DX11_WARP_TRIANGLE_OK\n" );
	return 0;
}
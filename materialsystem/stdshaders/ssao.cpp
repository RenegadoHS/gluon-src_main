//========= Copyright &copy; 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Crossroads devtest
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "ssao_vs30.inc"
#include "ssao_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
// SHADER PARAMS DEFINED IN SHADER FXC CODE
ConVar cr_ssao_samples( "cr_ssao_samples", "8" );
ConVar cr_ssao_contrast( "cr_ssao_contrast", "2.0" );
ConVar cr_ssao_radius( "cr_ssao_radius", "16" );
ConVar cr_ssao_bias( "cr_ssao_bias", "0.02" );
ConVar cr_ssao_bias_offset( "cr_ssao_bias_offset", "0.05" );
ConVar cr_ssao_illuminfluence( "cr_ssao_illuminfluence", "5.0" );
ConVar cr_ssao_zfar( "cr_ssao_zfar", "8.0" );
ConVar cr_ssao_znear( "cr_ssao_znear", "1.0" );

BEGIN_VS_SHADER_FLAGS( SSAO, "Help for SSAO", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "Framebuffer" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		if( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
	
			// Render targets are pegged as sRGB on POSIX, so just force these reads and writes
			//bool bForceSRGBReadAndWrite = 0 && g_pHardwareConfig->CanDoSRGBReadFromRTs();
			bool bForceSRGBReadAndWrite = 0; //No POSIX for now!
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, bForceSRGBReadAndWrite );
			pShaderShadow->EnableSRGBWrite( bForceSRGBReadAndWrite );
			
			DECLARE_STATIC_VERTEX_SHADER( ssao_vs30 );
			SET_STATIC_VERTEX_SHADER( ssao_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( ssao_ps30 );
			SET_STATIC_PIXEL_SHADER( ssao_ps30 );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );

			ITexture *src_texture = params[BASETEXTURE]->GetTextureValue();

			int width = src_texture->GetActualWidth();
			int height = src_texture->GetActualHeight();

			float g_TexelSize[2] = { 1.0f / float( width ), 1.0f / float( height ) };

			pShaderAPI->SetPixelShaderConstant( 0, g_TexelSize );
			
			DECLARE_DYNAMIC_VERTEX_SHADER( ssao_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( ssao_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( ssao_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( ssao_ps30 );

			float samples = cr_ssao_samples.GetInt();
			float contrast = cr_ssao_contrast.GetFloat();
			float radius = cr_ssao_radius.GetFloat();
			float bias = cr_ssao_bias.GetFloat();
			float biasoffset = cr_ssao_bias_offset.GetFloat();
			float illuminf = cr_ssao_illuminfluence.GetFloat();
			float zfar = cr_ssao_zfar.GetFloat();
			float znear = cr_ssao_znear.GetFloat();

			pShaderAPI->SetPixelShaderConstant( 1, &samples );
			pShaderAPI->SetPixelShaderConstant( 2, &radius );
			pShaderAPI->SetPixelShaderConstant( 3, &bias );
			pShaderAPI->SetPixelShaderConstant( 4, &illuminf );
			pShaderAPI->SetPixelShaderConstant( 5, &contrast );
			pShaderAPI->SetPixelShaderConstant( 6, &znear );
			pShaderAPI->SetPixelShaderConstant( 7, &zfar );
			pShaderAPI->SetPixelShaderConstant( 8, &biasoffset );
		}
		Draw();
	}
END_SHADER
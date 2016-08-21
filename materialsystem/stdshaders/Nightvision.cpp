#include "BaseVSShader.h"
#include "post_nightvision_ps20.inc"
#include "passthrough_vs20.inc" 

BEGIN_VS_SHADER(Nightvision, "Nightvision") 

BEGIN_SHADER_PARAMS
SHADER_PARAM(VISIONINTENSITY, SHADER_PARAM_TYPE_FLOAT, "1.0", "")
END_SHADER_PARAMS

SHADER_INIT
{
}

bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame) const
{
	return true;
}

SHADER_FALLBACK
{
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true); 
		pShaderShadow->EnableDepthWrites(false);  
		int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0); 
		pShaderShadow->SetVertexShader("passthrough_vs20", 0); 
		pShaderShadow->SetPixelShader("post_nightvision_ps20"); 

		DefaultFog();
	}

	DYNAMIC_STATE
	{
		float Scale = params[VISIONINTENSITY]->GetFloatValue(); 
		float vScale[4] = { Scale, Scale, Scale, 1 }; 
		pShaderAPI->SetPixelShaderConstant(0, vScale); 
		pShaderAPI->BindStandardTexture(SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0);
	}

	Draw();
}

END_SHADER
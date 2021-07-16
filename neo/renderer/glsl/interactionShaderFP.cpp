/*
 * This file is part of the D3wasm project (http://www.continuation-labs.com/projects/d3wasm)
 * Copyright (c) 2019 Gabriel Cuvillier.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "glsl_shaders.h"

const char * const interactionShaderFP = R"(
float4 main(
	float2 var_TexDiffuse : TEXCOORD0,
	float2 var_TexNormal : TEXCOORD1,
	float2 var_TexSpecular : TEXCOORD2,
	float4 var_TexLight : TEXCOORD3,
	float3 var_L : TEXCOORD4,
	float3 var_H : TEXCOORD5,
	float4 var_Color : COLOR,
	uniform float4 u_diffuseColor,
	uniform float4 u_specularColor,
	uniform sampler2D u_fragmentMap0, // u_bumpTexture
	uniform sampler2D u_fragmentMap1, // u_lightFalloffTexture
	uniform sampler2D u_fragmentMap2, // u_lightProjectionTexture
	uniform sampler2D u_fragmentMap3, // u_diffuseTexture
	uniform sampler2D u_fragmentMap4 // u_specularTexture
) {
  float3 L = normalize(var_L);
  float3 H = normalize(var_H);
  float3 N = 2.0f * tex2D(u_fragmentMap0, var_TexNormal).agb - 1.0f;
  
  float NdotL = clamp(dot(N, L), 0.0f, 1.0f);
  float NdotH = clamp(dot(N, H), 0.0f, 1.0f);

  float3 lightProjection = tex2Dproj(u_fragmentMap2, var_TexLight.xyw).rgb;
  float3 lightFalloff = tex2D(u_fragmentMap1, float2(var_TexLight.z, 0.5f)).rgb;
  float3 diffuseColor = tex2D(u_fragmentMap3, var_TexDiffuse).rgb * u_diffuseColor.rgb;
  float3 specularColor = 2.0f * tex2D(u_fragmentMap4, var_TexSpecular).rgb * u_specularColor.rgb;
  
  float specularFalloff = pow(NdotH, 12.0f);
  
  float3 color;
  color = diffuseColor;
  color += specularFalloff * specularColor;
  color *= NdotL * lightProjection;
  color *= lightFalloff;
  
  return float4(color, 1.0f) * var_Color;
}
)";

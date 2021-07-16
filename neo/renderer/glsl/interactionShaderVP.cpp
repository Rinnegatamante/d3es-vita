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

const char * const interactionShaderVP = R"(
void main(
	float4 attr_Vertex,
	float4 attr_Color,
	float4 attr_TexCoord,
	float3 attr_Tangent,
	float3 attr_Bitangent,
	float3 attr_Normal,
	uniform float4x4 u_modelViewProjectionMatrix,
	uniform float4x4 u_lightProjection,
	uniform float u_colorModulate,
	uniform float u_colorAdd,
	uniform float4 u_lightOrigin,
	uniform float4 u_viewOrigin,
	uniform float4 u_bumpMatrixS,
	uniform float4 u_bumpMatrixT,
	uniform float4 u_diffuseMatrixS,
	uniform float4 u_diffuseMatrixT,
	uniform float4 u_specularMatrixS,
	uniform float4 u_specularMatrixT,
	float2 out var_TexDiffuse : TEXCOORD0,
	float2 out var_TexNormal : TEXCOORD1,
	float2 out var_TexSpecular : TEXCOORD2,
	float4 out var_TexLight : TEXCOORD3,
	float3 out var_L : TEXCOORD4,
	float3 out var_H : TEXCOORD5,
	float4 out var_Color : COLOR,
	float4 out gl_Position : POSITION
) {
  float3x3 M = float3x3(attr_Tangent, attr_Bitangent, attr_Normal);
  
  var_TexNormal.x = dot(u_bumpMatrixS, attr_TexCoord);
  var_TexNormal.y = dot(u_bumpMatrixT, attr_TexCoord);
  
  var_TexDiffuse.x = dot(u_diffuseMatrixS, attr_TexCoord);
  var_TexDiffuse.y = dot(u_diffuseMatrixT, attr_TexCoord);
  
  var_TexSpecular.x = dot(u_specularMatrixS, attr_TexCoord);
  var_TexSpecular.y = dot(u_specularMatrixT, attr_TexCoord);
  
  var_TexLight.x = dot(u_lightProjection[0], attr_Vertex);
  var_TexLight.y = dot(u_lightProjection[1], attr_Vertex);
  var_TexLight.z = dot(u_lightProjection[2], attr_Vertex);
  var_TexLight.w = dot(u_lightProjection[3], attr_Vertex);
  
  float3 L = u_lightOrigin.xyz - attr_Vertex.xyz;
  float3 V = u_viewOrigin.xyz - attr_Vertex.xyz;
  float3 H = normalize(L) + normalize(V);
  
  var_L = mul(M, L);
  var_H = mul(M, H);

  var_Color = (attr_Color * u_colorModulate) + float4(u_colorAdd);
  
  gl_Position = mul(attr_Vertex, u_modelViewProjectionMatrix);
}
)";

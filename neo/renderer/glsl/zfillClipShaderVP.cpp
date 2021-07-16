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

const char * const zfillClipShaderVP = R"(  
void main(
	float4 attr_Vertex,
	float4 attr_TexCoord,
	uniform float4x4 u_modelViewProjectionMatrix,
	uniform float4x4 u_textureMatrix,
	uniform float4 u_clipPlane,
	float2 out var_TexDiffuse : TEXCOORD0,
	float2 out var_TexClip : TEXCOORD1,
	float4 out gl_Position : POSITION
) {
  var_TexDiffuse = mul(attr_TexCoord, u_textureMatrix).xy;  // Homogeneous coordinates of textureMatrix supposed to be 1

  var_TexClip = float2( dot( u_clipPlane, attr_Vertex), 0.5f );

  gl_Position = mul(attr_Vertex, u_modelViewProjectionMatrix);
}
)";

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

const char * const diffuseMapShaderVP = R"(
void main(
	float4 attr_Vertex,
	float4 attr_Color,
	float4 attr_TexCoord,
	uniform float4x4 u_modelViewProjectionMatrix,
	uniform float4x4 u_textureMatrix,
	uniform float u_colorAdd,
	uniform float u_colorModulate,
	float2 out var_TexCoord : TEXCOORD0,
	float4 out var_Color : COLOR,
	float4 out gl_Position : POSITION
) {
  var_TexCoord = mul(attr_TexCoord, u_textureMatrix).xy;
  
  if (u_colorModulate == 0.0f) {
    var_Color = float4(u_colorAdd);
  } else {
    var_Color = (attr_Color * u_colorModulate) + float4(u_colorAdd);
  }
  
  gl_Position = mul(attr_Vertex, u_modelViewProjectionMatrix);
}
)";

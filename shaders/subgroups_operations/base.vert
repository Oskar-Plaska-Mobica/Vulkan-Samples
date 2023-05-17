#version 450
/* Copyright (c) 2023, Mobica Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUv;

layout (binding = 0) uniform Ubo 
{
	mat4 projection;
	mat4 model;
	vec4 view;
} ubo;

void main() 
{
	outUv = inUV;
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0f);
}

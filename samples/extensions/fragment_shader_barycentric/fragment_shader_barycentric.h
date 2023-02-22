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

#pragma once

#include "rendering/render_pipeline.h"
#include "scene_graph/components/camera.h"
#include "api_vulkan_sample.h"

class FragmentShaderBarycentric : public ApiVulkanSample
{
  public:
	struct
	{
		Texture envmap;
	} textures;

	struct UBOVS
	{
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 skybox_modelview;
		float     modelscale = 0.05f;
	} ubo_vs;

	std::unique_ptr<vkb::sg::SubMesh>  skybox;
	std::unique_ptr<vkb::sg::SubMesh>  object;
	std::unique_ptr<vkb::core::Buffer> ubo;

	VkPipeline            model_pipeline{VK_NULL_HANDLE};
	VkPipeline            skybox_pipeline{VK_NULL_HANDLE};
	VkPipelineLayout      pipeline_layout{VK_NULL_HANDLE};
	VkDescriptorSet       descriptor_set{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
	VkDescriptorPool      descriptor_pool{VK_NULL_HANDLE};

	VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR fragment_shader_barycentric_properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR};
	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR  fragment_shader_barycentric_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR};


	FragmentShaderBarycentric();
	~FragmentShaderBarycentric();

	bool prepare(vkb::Platform &platform) override;

	void render(float delta_time) override;
	void build_command_buffers() override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;

  private:
	void load_assets();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void setup_descriptor_set_layout();
	void create_descriptor_sets();
	void create_descriptor_pool();
	void create_pipeline();
	void draw();
};

std::unique_ptr<vkb::VulkanSample> create_fragment_shader_barycentric();
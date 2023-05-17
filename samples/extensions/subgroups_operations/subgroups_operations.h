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

#include "api_vulkan_sample.h"

struct TextureQuadVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
};

class SubgroupsOperations : public ApiVulkanSample
{
  public:
	SubgroupsOperations();
	~SubgroupsOperations();

	bool prepare(vkb::Platform &platform) override;
	void request_gpu_features(vkb::PhysicalDevice &gpu) override;
	void build_command_buffers() override;
	void render(float delta_time) override;
	bool resize(const uint32_t width, const uint32_t height) override;
	void on_update_ui_overlay(vkb::Drawer &drawer) override;

	void draw();
	void load_assets();

	void generate_quad();
	void setup_descriptor_pool();
	void setup_descriptor_set_layout();
	void setup_descriptor_set();
	void setup_pipelines();
	void prepare_uniform_buffers();
	void update_uniform_buffers();
	void create_command_buffers();

	struct GuiSettings
	{
		int32_t                         selected_filtr = 0;
		static std::vector<std::string> init_filters_name()
		{
			std::vector<std::string> filtrs = {
			    {"Blur"}, {"Sharpen"}, {"Edge detection vertical, horizontal, and diagonal"}, {"Canny edge"}};
			return filtrs;
		}
	} guiSettings;

	Texture                            texture;
	std::unique_ptr<vkb::core::Buffer> vertex_buffer;
	std::unique_ptr<vkb::core::Buffer> index_buffer;
	uint32_t                           index_count;

	std::unique_ptr<vkb::core::Buffer> uniform_buffer_vs;

	struct
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
	} ubo_vs;

	VkPipeline            texture_pipeline;
	VkPipelineLayout      pipeline_layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;

	// compute
	const vkb::Queue* compute_queue;
	std::vector<uint32_t> queue_families;
};

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations();

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

#include "subgroups_operations.h"

void SubgroupsOperations::Pipeline::destroy(VkDevice device)
{
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
}

SubgroupsOperations::SubgroupsOperations()
{
	title       = "Subgroups operations";
	camera.type = vkb::CameraType::LookAt;

	camera.set_perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 256.0f);
	add_device_extension(VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);
	add_device_extension(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME);        // is needed???
	add_device_extension(VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME);          // is needed???
}

SubgroupsOperations::~SubgroupsOperations()
{
	if (device)
	{
		compute.pipelines._default.destroy(get_device().get_handle());
		vkDestroyDescriptorSetLayout(get_device().get_handle(), compute.descriptor_set_layout, nullptr);
		vkDestroySemaphore(get_device().get_handle(), compute.semaphore, nullptr);
		vkDestroyCommandPool(get_device().get_handle(), compute.command_pool, nullptr);
		vkDestroyDescriptorPool(get_device().get_handle(), descriptor_pool, nullptr);
	}
}

bool SubgroupsOperations::prepare(vkb::Platform &platform)
{
	if (!ApiVulkanSample::prepare(platform))
	{
		return false;
	}

	load_assets();
	setup_descriptor_pool();
	prepare_compute();
	prepare_graphics();

	build_compute_command_buffer();

	prepared = true;
	return true;
}

void SubgroupsOperations::prepare_compute()
{
	create_compute_queue();
	create_compute_command_pool();
	create_compute_command_buffer();
	create_compute_descriptor_set_layout();
	create_compute_descriptor_set();
	preapre_compute_pipeline_layout();
	prepare_compute_pipeline();
}

void SubgroupsOperations::create_compute_queue()
{
	// create compute queue and get family index
	compute.queue_family_index = get_device().get_queue_family_index(VK_QUEUE_COMPUTE_BIT);

	vkGetDeviceQueue(get_device().get_handle(), compute.queue_family_index, 0, &compute.queue);
}

void SubgroupsOperations::create_compute_command_pool()
{
	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex        = compute.queue_family_index;
	command_pool_create_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK(vkCreateCommandPool(get_device().get_handle(), &command_pool_create_info, nullptr, &compute.command_pool));
}

void SubgroupsOperations::create_compute_command_buffer()
{
	// Create a command buffer for compute operations
	VkCommandBufferAllocateInfo command_buffer_allocate_info =
	    vkb::initializers::command_buffer_allocate_info(
	        compute.command_pool,
	        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	        1);

	VK_CHECK(vkAllocateCommandBuffers(get_device().get_handle(), &command_buffer_allocate_info, &compute.command_buffer));

	// Semaphore for compute & graphics sync
	VkSemaphoreCreateInfo semaphore_create_info = vkb::initializers::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(get_device().get_handle(), &semaphore_create_info, nullptr, &compute.semaphore));
}

void SubgroupsOperations::create_compute_descriptor_set_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> set_layout_bindngs = {
	    vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 0)};
	VkDescriptorSetLayoutCreateInfo descriptor_layout = vkb::initializers::descriptor_set_layout_create_info(set_layout_bindngs);

	VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &compute.descriptor_set_layout));
}

void SubgroupsOperations::create_compute_descriptor_set()
{
	VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &compute.descriptor_set_layout, 1);

	VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &compute.descriptor_set));

	//// TODO: add write descriptors
	// std::vector<VkWriteDescriptorSet> compute_write_descriptor_sets = {

	//};

	// vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(compute_write_descriptor_sets.size()), compute_write_descriptor_sets.data(), 0, nullptr);
}

void SubgroupsOperations::preapre_compute_pipeline_layout()
{
	VkPipelineLayoutCreateInfo compute_pipeline_layout_info = {};
	compute_pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	compute_pipeline_layout_info.setLayoutCount             = 1;
	compute_pipeline_layout_info.pSetLayouts                = &compute.descriptor_set_layout;

	VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &compute_pipeline_layout_info, nullptr, &compute.pipelines._default.pipeline_layout));
}

void SubgroupsOperations::prepare_compute_pipeline()
{
	VkComputePipelineCreateInfo computeInfo = {};
	computeInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computeInfo.layout                      = compute.pipelines._default.pipeline_layout;
	computeInfo.stage                       = load_shader("subgroups_operations/test.comp", VK_SHADER_STAGE_COMPUTE_BIT);

	VK_CHECK(vkCreateComputePipelines(get_device().get_handle(), pipeline_cache, 1, &computeInfo, nullptr, &compute.pipelines._default.pipeline));
}

void SubgroupsOperations::build_compute_command_buffer()
{
	VkSubmitInfo submit_info = {};
	submit_info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkResetCommandBuffer(compute.command_buffer, 0);

	// record command
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK(vkBeginCommandBuffer(compute.command_buffer, &begin_info));

	vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelines._default.pipeline);
	vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelines._default.pipeline_layout, 0, 1, &compute.descriptor_set, 0, nullptr);
	vkCmdDispatch(compute.command_buffer, 256, 1, 1);

	VK_CHECK(vkEndCommandBuffer(compute.command_buffer));
}

void SubgroupsOperations::prepare_graphics()
{
	prepare_uniform_buffers();
	setup_descriptor_set_layout();
	setup_pipelines();
	setup_descriptor_set();
	create_command_buffers();
	build_command_buffers();
}

void SubgroupsOperations::request_gpu_features(vkb::PhysicalDevice &gpu)
{
	if (gpu.get_features().samplerAnisotropy)
	{
		gpu.get_mutable_requested_features().samplerAnisotropy = VK_TRUE;
	}
	subgroups_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
	subgroups_properties.pNext = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties2 device_properties2 = {};
	device_properties2.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties2.pNext                       = &subgroups_properties;
	vkGetPhysicalDeviceProperties2(gpu.get_handle(), &device_properties2);
}

void SubgroupsOperations::prepare_uniform_buffers()
{
	update_uniform_buffers();
}

void SubgroupsOperations::update_uniform_buffers()
{
}

void SubgroupsOperations::setup_pipelines()
{
}

void SubgroupsOperations::setup_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {

	};

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = vkb::initializers::descriptor_pool_create_info(pool_sizes, 4);
	VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void SubgroupsOperations::setup_descriptor_set_layout()
{
}

void SubgroupsOperations::setup_descriptor_set()
{
}

void SubgroupsOperations::load_assets()
{
}

void SubgroupsOperations::create_command_buffers()
{
}

void SubgroupsOperations::build_command_buffers()
{
	VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

	std::array<VkClearValue, 2> clear_values;
	clear_values[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
	clear_values[1].depthStencil = {0.0f, 0};

	VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
	render_pass_begin_info.renderPass               = render_pass;
	render_pass_begin_info.renderArea.extent.width  = width;
	render_pass_begin_info.renderArea.extent.height = height;
	render_pass_begin_info.clearValueCount          = static_cast<uint32_t>(clear_values.size());
	render_pass_begin_info.pClearValues             = clear_values.data();

	for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
	{
		render_pass_begin_info.framebuffer = framebuffers[i];

		VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

		vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
		vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

		VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

		vkCmdEndRenderPass(draw_cmd_buffers[i]);

		VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
	}
}

void SubgroupsOperations::draw()
{
	VkPipelineStageFlags graphics_wait_stage_masks[]  = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore          graphics_wait_semaphores[]   = {compute.semaphore, semaphores.acquired_image_ready};
	VkSemaphore          graphics_signal_semaphores[] = {semaphore, semaphores.render_complete};

	ApiVulkanSample::prepare_frame();
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &draw_cmd_buffers[current_buffer];
	submit_info.waitSemaphoreCount   = 2;
	submit_info.pWaitSemaphores      = graphics_wait_semaphores;
	submit_info.pWaitDstStageMask    = graphics_wait_stage_masks;
	submit_info.signalSemaphoreCount = 2;
	submit_info.pSignalSemaphores    = graphics_signal_semaphores;
	VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
	ApiVulkanSample::submit_frame();

		// Wait for rendering finished
	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	// Submit compute commands
	VkSubmitInfo compute_submit_info         = vkb::initializers::submit_info();
	compute_submit_info.commandBufferCount   = 1;
	compute_submit_info.pCommandBuffers      = &compute.command_buffer;
	compute_submit_info.waitSemaphoreCount   = 1;
	compute_submit_info.pWaitSemaphores      = &semaphore;
	compute_submit_info.pWaitDstStageMask    = &wait_stage_mask;
	compute_submit_info.signalSemaphoreCount = 1;
	compute_submit_info.pSignalSemaphores    = &compute.semaphore;

	VK_CHECK(vkQueueSubmit(compute.queue, 1, &compute_submit_info, VK_NULL_HANDLE));
}

void SubgroupsOperations::on_update_ui_overlay(vkb::Drawer &drawer)
{
	if (drawer.header("Settings"))
	{
		if (drawer.combo_box("Filters", &gui_settings.selected_filtr, gui_settings.init_filters_name()))
		{
			update_uniform_buffers();
		}
	}
}

bool SubgroupsOperations::resize(const uint32_t width, const uint32_t height)
{
	if (!ApiVulkanSample::resize(width, height))
		return false;
	build_command_buffers();
	update_uniform_buffers();
	return true;
}

void SubgroupsOperations::render(float delta_time)
{
	if (!prepared)
	{
		return;
	}
	draw();
	if (camera.updated)
	{
		update_uniform_buffers();
	}
}

std::unique_ptr<vkb::VulkanSample> create_subgroups_operations()
{
	return std::make_unique<SubgroupsOperations>();
}

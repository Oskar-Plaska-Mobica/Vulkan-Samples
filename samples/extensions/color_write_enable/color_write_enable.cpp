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

#include "color_write_enable.h"

ColorWriteEnable::ColorWriteEnable()
{
    add_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    add_device_extension(VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME);
    add_device_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
}

ColorWriteEnable::~ColorWriteEnable()
{
    if (device)
    {
        vkDestroyPipeline(get_device().get_handle(), triangle_pipeline, nullptr);
        vkDestroyPipelineLayout(get_device().get_handle(), triangle_pipeline_layout, nullptr);
    }
}

bool ColorWriteEnable::prepare(vkb::Platform &platform)
{
    if (!ApiVulkanSample::prepare(platform))
    {
        return false;
    }

    prepare_pipelines();
    build_command_buffers();
    prepared = true;
    return true;
}

void ColorWriteEnable::prepare_pipelines()
{
    // Create a blank pipeline layout.
    // We are not binding any resources to the pipeline in this sample.
    VkPipelineLayoutCreateInfo layout_info = vkb::initializers::pipeline_layout_create_info(nullptr, 0);
    VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &triangle_pipeline_layout));

    VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();

    // Specify we will use triangle lists to draw geometry.
    VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    // Specify rasterization state.
    VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

    // Our attachment will write to all color channels, but no blending is enabled.
    VkPipelineColorBlendAttachmentState blend_attachment = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

    VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment);

    VkPipelineColorWriteCreateInfoEXT colorWriteInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT};
    const VkBool32 colorWriteEnables[1] = { write_enabled };
    colorWriteInfo.pColorWriteEnables = colorWriteEnables;

    color_blend_state.pNext = &colorWriteInfo;

    // We will have one viewport and scissor box.
    VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

    // Disable all depth testing.
    VkPipelineDepthStencilStateCreateInfo depth_stencil = vkb::initializers::pipeline_depth_stencil_state_create_info(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER);

    // No multisampling.
    VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

    // Specify that these states will be dynamic, i.e. not part of pipeline state object.
    std::array<VkDynamicState, 3>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT};
    VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));

    // Load our SPIR-V shaders.
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

    // Vertex stage of the pipeline
    shader_stages[0] = load_shader("triangle.vert", VK_SHADER_STAGE_VERTEX_BIT);
    shader_stages[1] = load_shader("triangle.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

    // We need to specify the pipeline layout and the render pass description up front as well.
    VkGraphicsPipelineCreateInfo pipe = vkb::initializers::pipeline_create_info(triangle_pipeline_layout, render_pass);
    pipe.stageCount                   = vkb::to_u32(shader_stages.size());
    pipe.pStages                      = shader_stages.data();
    pipe.pVertexInputState            = &vertex_input;
    pipe.pInputAssemblyState          = &input_assembly;
    pipe.pRasterizationState          = &raster;
    pipe.pColorBlendState             = &color_blend_state;
    pipe.pMultisampleState            = &multisample;
    pipe.pViewportState               = &viewport;
    pipe.pDepthStencilState           = &depth_stencil;
    pipe.pDynamicState                = &dynamic;

    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &triangle_pipeline));
}

void ColorWriteEnable::build_command_buffers()
{
    VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

    // Clear color and depth values.
    VkClearValue clear_values[2];
    clear_values[0].color        = {{0.0f, 0.0f, 0.1f, 0.0f}};
    clear_values[1].depthStencil = {0.0f, 0};

    // Begin the render pass.
    VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
    render_pass_begin_info.renderPass               = render_pass;
    render_pass_begin_info.renderArea.offset.x      = 0;
    render_pass_begin_info.renderArea.offset.y      = 0;
    render_pass_begin_info.renderArea.extent.width  = width;
    render_pass_begin_info.renderArea.extent.height = height;
    render_pass_begin_info.clearValueCount          = 2;
    render_pass_begin_info.pClearValues             = clear_values;

    for (int32_t i = 0; i < draw_cmd_buffers.size(); ++i)
    {
        auto cmd = draw_cmd_buffers[i];

        // Begin command buffer.
        vkBeginCommandBuffer(cmd, &command_buffer_begin_info);

        // Set framebuffer for this command buffer.
        render_pass_begin_info.framebuffer = framebuffers[i];
        // We will add draw commands in the same command buffer.
        vkCmdBeginRenderPass(cmd, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // Bind the graphics pipeline.
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline);

        // Set viewport dynamically
        VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        // Set scissor dynamically
        VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Toggle color_write_enable dynamically
        VkBool32 colorWriteEnables[1] = { write_enabled };
        vkCmdSetColorWriteEnableEXT(cmd, 1, colorWriteEnables );

        // Draw three vertices with one instance.
        vkCmdDraw(cmd, 3, 1, 0, 0);

        // Draw user interface.
        draw_ui(cmd);

        // Complete render pass.
        vkCmdEndRenderPass(cmd);

        // Complete the command buffer.
        VK_CHECK(vkEndCommandBuffer(cmd));
    }
}

void ColorWriteEnable::on_update_ui_overlay(vkb::Drawer &drawer)
{
    if (drawer.header("Color write"))
    {

        if (drawer.checkbox("Enabled", &write_enabled))
        {
            build_command_buffers();
        }
    }
}

void ColorWriteEnable::render(float delta_time)
{
    if (!prepared)
    {
        return;
    }
    ApiVulkanSample::prepare_frame();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &draw_cmd_buffers[current_buffer];
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
    ApiVulkanSample::submit_frame();
}


std::unique_ptr<vkb::Application> create_color_write_enable()
{
    return std::make_unique<ColorWriteEnable>();
}

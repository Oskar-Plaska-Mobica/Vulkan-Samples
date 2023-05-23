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
        vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.color, nullptr);
        vkDestroyPipeline(get_device().get_handle(), pipelines.color, nullptr);
        vkDestroyPipelineLayout(get_device().get_handle(), pipeline_layouts.composition, nullptr);
        vkDestroyPipeline(get_device().get_handle(), pipelines.composition, nullptr);
        //vkDestroyPipeline(get_device().get_handle(), green_pipeline, nullptr);
        //vkDestroyPipeline(get_device().get_handle(), blue_pipeline, nullptr);
        //vkDestroyPipelineLayout(get_device().get_handle(), triangle_pipeline_layout, nullptr);
    }

}

void ColorWriteEnable::create_attachment(VkFormat format, FrameBufferAttachment *attachment)
{

    VkImageAspectFlags aspect_mask = 0;
    VkImageLayout      image_layout;

    attachment->format = format;

    //    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    //    {
    aspect_mask  = VK_IMAGE_ASPECT_COLOR_BIT;
    image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //    }
    //    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    //    {
    //        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
    //        // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    //        if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
    //        {
    //            aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    //        }
    //        image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //    }

    assert(aspect_mask > 0);

    VkImageCreateInfo image = vkb::initializers::image_create_info();
    image.imageType         = VK_IMAGE_TYPE_2D;
    image.format            = format;
    image.extent.width      = get_render_context().get_surface_extent().width;
    image.extent.height     = get_render_context().get_surface_extent().height;
    image.extent.depth      = 1;
    image.mipLevels         = 1;
    image.arrayLayers       = 1;
    image.samples           = VK_SAMPLE_COUNT_1_BIT;
    image.tiling            = VK_IMAGE_TILING_OPTIMAL;
    image.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo memory_allocate_info = vkb::initializers::memory_allocate_info();
    VkMemoryRequirements memory_requirements;

    VK_CHECK(vkCreateImage(get_device().get_handle(), &image, nullptr, &attachment->image));
    vkGetImageMemoryRequirements(get_device().get_handle(), attachment->image, &memory_requirements);
    memory_allocate_info.allocationSize  = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = get_device().get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(get_device().get_handle(), &memory_allocate_info, nullptr, &attachment->mem));
    VK_CHECK(vkBindImageMemory(get_device().get_handle(), attachment->image, attachment->mem, 0));

    VkImageViewCreateInfo image_view_create_info           = vkb::initializers::image_view_create_info();
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = format;
    image_view_create_info.subresourceRange                = {};
    image_view_create_info.subresourceRange.aspectMask     = aspect_mask;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;
    image_view_create_info.image                           = attachment->image;
    VK_CHECK(vkCreateImageView(get_device().get_handle(), &image_view_create_info, nullptr, &attachment->view));

}

void ColorWriteEnable::create_attachments() {
    create_attachment(VK_FORMAT_B8G8R8A8_SRGB, &attachments.red);
    create_attachment(VK_FORMAT_B8G8R8A8_SRGB, &attachments.green);
    create_attachment(VK_FORMAT_B8G8R8A8_SRGB, &attachments.blue);
}

bool ColorWriteEnable::prepare(vkb::Platform &platform)
{
    if (!ApiVulkanSample::prepare(platform))
    {
        return false;
    }
    setup_descriptor_set_layout();
    prepare_pipelines();
    setup_descriptor_pool();
    setup_descriptor_set();
    build_command_buffers();
    prepared = true;
    return true;
}

void ColorWriteEnable::prepare_pipelines()
{

    {
        // Create a blank pipeline layout.
        // We are not binding any resources to the pipeline in this sample.
        VkPipelineLayoutCreateInfo layout_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.color, 1);
        VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &layout_info, nullptr, &pipeline_layouts.color));

        VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();

        // Specify we will use triangle lists to draw geometry.
        VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        // Specify rasterization state.
        VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

        // Prepare separate attachment for each color channel.
        VkPipelineColorBlendAttachmentState blend_attachment_state   = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);


        VkPipelineColorBlendAttachmentState red_blend_attachment   = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        VkPipelineColorBlendAttachmentState green_blend_attachment = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        VkPipelineColorBlendAttachmentState blue_blend_attachment  = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        VkPipelineColorBlendAttachmentState blend_attachment[3]    = /*{blend_attachment_state};//*/{red_blend_attachment, green_blend_attachment, blue_blend_attachment};

        // Define separate color_write_enable toggle for each color attachment.
        VkPipelineColorWriteCreateInfoEXT color_write_info    = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT};
        std::array<VkBool32, 3>           color_write_enables = {r_bit_enabled, g_bit_enabled, b_bit_enabled};
        color_write_info.attachmentCount                      = static_cast<uint32_t>(color_write_enables.size());
        color_write_info.pColorWriteEnables                   = color_write_enables.data();


        // Define color blend with an attachment for each color. Chain it with color_write_info.
        VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(3, blend_attachment);

        color_blend_state.pNext                               = &color_write_info;

        // We will have one viewport and scissor box.
        VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

        // No multisampling.
        VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

        // Specify that these states will be dynamic, i.e. not part of pipeline state object.
        std::array<VkDynamicState, 3>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT};
        VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));

        // Load our SPIR-V shaders.
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};

        // Vertex stage of the pipeline
        shader_stages[0] = load_shader("color_write_enable/triangle_separate_channels.vert", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = load_shader("color_write_enable/triangle_separate_channels.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

        // We need to specify the pipeline layout and the render pass description up front as well.
        VkGraphicsPipelineCreateInfo pipe = vkb::initializers::pipeline_create_info(pipeline_layouts.color, render_pass);
        pipe.stageCount                   = vkb::to_u32(shader_stages.size());
        pipe.pStages                      = shader_stages.data();
        pipe.pVertexInputState            = &vertex_input;
        pipe.pInputAssemblyState          = &input_assembly;
        pipe.pRasterizationState          = &raster;
        pipe.pColorBlendState             = &color_blend_state;
        pipe.pMultisampleState            = &multisample;
        pipe.pViewportState               = &viewport;
        pipe.pDynamicState                = &dynamic;
        pipe.subpass = 0;
        //blend_attachment[0] = red_blend_attachment;
        //pipe.pColorBlendState = &color_blend_state;
        VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &pipelines.color));

        //    pipe.subpass = 1;
        //    blend_attachment[0] = red_blend_attachment;
        //    pipe.pColorBlendState = &color_blend_state;
        //    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &red_pipeline));

        //    pipe.subpass = 2;
        //    blend_attachment[0] = green_blend_attachment;
        //    pipe.pColorBlendState = &color_blend_state;
        //    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &green_pipeline));

        //    pipe.subpass = 3;
        //    blend_attachment[0] = blue_blend_attachment;
        //    pipe.pColorBlendState = &color_blend_state;
        //    VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &blue_pipeline));

    }
    {
        VkPipelineLayoutCreateInfo composition_layout_info = vkb::initializers::pipeline_layout_create_info(&descriptor_set_layouts.composition, 1);
        VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &composition_layout_info, nullptr, &pipeline_layouts.composition));

        VkPipelineVertexInputStateCreateInfo vertex_input = vkb::initializers::pipeline_vertex_input_state_create_info();


        // Load our SPIR-V shaders.
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};

        // Vertex stage of the pipeline
        shader_stages[0] = load_shader("color_write_enable/composition.vert", VK_SHADER_STAGE_VERTEX_BIT);
        shader_stages[1] = load_shader("color_write_enable/composition.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

        // Specify we will use triangle lists to draw geometry.
        VkPipelineInputAssemblyStateCreateInfo input_assembly = vkb::initializers::pipeline_input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

        // Specify rasterization state.
        VkPipelineRasterizationStateCreateInfo raster = vkb::initializers::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

        // Prepare separate attachment for each color channel.
        VkPipelineColorBlendAttachmentState blend_attachment_state   = vkb::initializers::pipeline_color_blend_attachment_state(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo color_blend_state = vkb::initializers::pipeline_color_blend_state_create_info(1, &blend_attachment_state);

        // We will have one viewport and scissor box.
        VkPipelineViewportStateCreateInfo viewport = vkb::initializers::pipeline_viewport_state_create_info(1, 1);

        // No multisampling.
        VkPipelineMultisampleStateCreateInfo multisample = vkb::initializers::pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);

        std::array<VkDynamicState, 2>    dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic = vkb::initializers::pipeline_dynamic_state_create_info(dynamics.data(), vkb::to_u32(dynamics.size()));


        VkGraphicsPipelineCreateInfo pipe = vkb::initializers::pipeline_create_info(pipeline_layouts.color, render_pass);
        pipe.stageCount                   = vkb::to_u32(shader_stages.size());
        pipe.pStages                      = shader_stages.data();
        pipe.pVertexInputState            = &vertex_input;
        pipe.pInputAssemblyState          = &input_assembly;
        pipe.pRasterizationState          = &raster;
        pipe.pColorBlendState             = &color_blend_state;
        pipe.pMultisampleState            = &multisample;
        pipe.pViewportState               = &viewport;
        pipe.pDynamicState                = &dynamic;
        pipe.subpass = 1;

        VK_CHECK(vkCreateGraphicsPipelines(get_device().get_handle(), pipeline_cache, 1, &pipe, nullptr, &pipelines.composition));
    }
}

void ColorWriteEnable::request_gpu_features(vkb::PhysicalDevice &gpu)
{
    {
        auto &features            = gpu.request_extension_features<VkPhysicalDeviceColorWriteEnableFeaturesEXT>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT);
        features.colorWriteEnable = VK_TRUE;
    }
    {
        auto &features = gpu.get_mutable_requested_features();
        features.independentBlend = VK_TRUE;
    }
}

void ColorWriteEnable::setup_descriptor_pool()
{

    std::vector<VkDescriptorPoolSize> poolSizes =
    {
        vkb::initializers::descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)
    };

    VkDescriptorPoolCreateInfo descriptorPoolInfo =
            vkb::initializers::descriptor_pool_create_info(
                static_cast<uint32_t>(poolSizes.size()),
                poolSizes.data(),
                1);

    VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptorPoolInfo, nullptr, &descriptor_pool));

}

void ColorWriteEnable::setup_descriptor_set_layout() {

    std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        vkb::initializers::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2)
    };

    VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info =
            vkb::initializers::descriptor_set_layout_create_info(set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

    VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.color));
    VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout_create_info, nullptr, &descriptor_set_layouts.composition));
}

void ColorWriteEnable::setup_descriptor_set() {
    VkDescriptorSetAllocateInfo alloc_info = vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layouts.composition, 1);
    VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &alloc_info, &descriptor_sets.composition));


    VkDescriptorImageInfo red = vkb::initializers::descriptor_image_info(samplers.red, attachments.red.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkDescriptorImageInfo green = vkb::initializers::descriptor_image_info(samplers.green, attachments.green.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkDescriptorImageInfo blue = vkb::initializers::descriptor_image_info(samplers.blue, attachments.blue.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        vkb::initializers::write_descriptor_set(descriptor_sets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &red),
        vkb::initializers::write_descriptor_set(descriptor_sets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &green),
        vkb::initializers::write_descriptor_set(descriptor_sets.composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &blue),

    };

    vkUpdateDescriptorSets(get_device().get_handle(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
}

void ColorWriteEnable::setup_render_pass()
{
    create_attachments();

    std::array<VkAttachmentDescription, 4> attachments = {};
    // color attachments
    for (uint32_t i = 0; i < attachments.size(); ++i)
    {
        attachments[i].format         = render_context->get_format();
        attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[i].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    /**
    std::array<VkAttachmentReference, 4> color_reference = {};
    for (uint32_t i = 0; i < color_reference.size(); ++i)
    {
        color_reference[i].attachment = i;
        color_reference[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    */

    // 1/3/4 color references?
    VkAttachmentReference color_references[4];
    color_references[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    color_references[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    color_references[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    color_references[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    std::array<VkSubpassDescription, 2> subpass_descriptions{};
    subpass_descriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_descriptions[0].colorAttachmentCount = 3;
    subpass_descriptions[0].pColorAttachments = color_references+1;
    subpass_descriptions[0].pDepthStencilAttachment = nullptr;


    VkAttachmentReference inputReferences[3];
    inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    subpass_descriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_descriptions[1].colorAttachmentCount = 1;
    subpass_descriptions[1].pColorAttachments = color_references;
    subpass_descriptions[1].pDepthStencilAttachment = nullptr;
    subpass_descriptions[1].inputAttachmentCount = 3;
    subpass_descriptions[1].pInputAttachments = inputReferences;


    //    for (uint32_t i = 0; i < subpass_descriptions.size(); ++i) {
    //        subpass_descriptions[i].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //        subpass_descriptions[i].colorAttachmentCount    = static_cast<uint32_t>(color_reference.size());
    //        subpass_descriptions[i].pColorAttachments       = color_reference.data();
    //        subpass_descriptions[i].inputAttachmentCount    = 0;
    //        subpass_descriptions[i].pInputAttachments       = nullptr;
    //        subpass_descriptions[i].preserveAttachmentCount = 0;
    //        subpass_descriptions[i].pPreserveAttachments    = nullptr;
    //        subpass_descriptions[i].pResolveAttachments     = nullptr;
    //    }

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies = {};

    //    for (uint32_t i = 0; i < dependencies.size(); ++i) {
    //        dependencies[i].srcSubpass      = i-1;
    //        dependencies[i].dstSubpass      = i;
    //        dependencies[i].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //        dependencies[i].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    //        dependencies[i].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    //        dependencies[i].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //        dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    //    }

    //    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    //    dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //    dependencies[1].srcSubpass = 0;
    //    dependencies[1].dstSubpass = 1;
    //    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //    dependencies[2].srcSubpass = 1;
    //    dependencies[2].dstSubpass = 2;
    //    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //    dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount        = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments           = attachments.data();
    render_pass_create_info.subpassCount           = static_cast<uint32_t>(subpass_descriptions.size());
    render_pass_create_info.pSubpasses             = subpass_descriptions.data();
    render_pass_create_info.dependencyCount        = static_cast<uint32_t>(dependencies.size());
    render_pass_create_info.pDependencies          = dependencies.data();

    VK_CHECK(vkCreateRenderPass(device->get_handle(), &render_pass_create_info, nullptr, &render_pass));

    VkSamplerCreateInfo sampler = vkb::initializers::sampler_create_info();
    sampler.magFilter           = VK_FILTER_NEAREST;
    sampler.minFilter           = VK_FILTER_NEAREST;
    sampler.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV        = sampler.addressModeU;
    sampler.addressModeW        = sampler.addressModeU;
    sampler.mipLodBias          = 0.0f;
    sampler.maxAnisotropy       = 1.0f;
    sampler.minLod              = 0.0f;
    sampler.maxLod              = 1.0f;
    sampler.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &samplers.red));
    VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &samplers.green));
    VK_CHECK(vkCreateSampler(get_device().get_handle(), &sampler, nullptr, &samplers.blue));
}

void ColorWriteEnable::setup_framebuffer()
{   /**
    //TODO CREATE ATTACHMENTS HERE

    std::vector< VkDescriptorImageInfo> descriptor_image_infos = {
        vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, red_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, green_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        vkb::initializers::descriptor_image_info(VK_NULL_HANDLE, blue_attachment.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    };

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    //VkDescriptorSet scene;
    for (size_t i = 0; i < descriptor_image_infos.size(); i++) {
        write_descriptor_sets.push_back(vkb::initializers::write_descriptor_set(scene_descriptor_set, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, i, &descriptor_image_infos[i]));
    }

    vkUpdateDescriptorSets(device->get_handle(), static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

    std::array<VkImageView, 4> attachments = {};

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext                   = NULL;
    framebuffer_create_info.renderPass              = render_pass;
    framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
    framebuffer_create_info.pAttachments            = attachments.data();
    framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
    framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
    framebuffer_create_info.layers                  = 1;

    // Delete existing frame buffers
    for (uint32_t i = 0; i < framebuffers.size(); i++)
    {
        if (framebuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device->get_handle(), framebuffers[i], nullptr);
        }
    }

    // Create frame buffers for every swap chain image
    framebuffers.resize(render_context->get_render_frames().size());
    for (uint32_t i = 0; i < framebuffers.size(); i++)
    {
        attachments[0] = red_attachment.view;//swapchain_buffers[i].view;
        attachments[1] = green_attachment.view;//swapchain_buffers[i].view;
        attachments[2] = blue_attachment.view;//swapchain_buffers[i].view;
        attachments[3] = swapchain_buffers[i].view;
        VK_CHECK(vkCreateFramebuffer(device->get_handle(), &framebuffer_create_info, nullptr, &framebuffers[i]));
    }
    */

    std::array<VkImageView, 4> attachments;

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext                   = NULL;
    framebuffer_create_info.renderPass              = render_pass;
    framebuffer_create_info.attachmentCount         = static_cast<uint32_t>(attachments.size());
    framebuffer_create_info.pAttachments            = attachments.data();
    framebuffer_create_info.width                   = get_render_context().get_surface_extent().width;
    framebuffer_create_info.height                  = get_render_context().get_surface_extent().height;
    framebuffer_create_info.layers                  = 1;

    framebuffers.resize(render_context->get_render_frames().size());
    for (uint32_t i = 0; i < framebuffers.size(); i++)
    {
        attachments[0] = swapchain_buffers[i].view;
        attachments[1] = this->attachments.red.view;
        attachments[2] = this->attachments.green.view;
        attachments[3] = this->attachments.blue.view;
        VK_CHECK(vkCreateFramebuffer(device->get_handle(), &framebuffer_create_info, nullptr, &framebuffers[i]));
    }
}

void ColorWriteEnable::build_command_buffers()
{

    VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

    // Clear color values.
    std::array<VkClearValue, 4> clear_values = {};
    for (int32_t i = 0; i < clear_values.size(); ++i)
        clear_values[i].color                    = {background_r_value, background_g_value, background_b_value, 0.0f};

    // Begin the render pass.
    VkRenderPassBeginInfo render_pass_begin_info    = vkb::initializers::render_pass_begin_info();
    render_pass_begin_info.renderPass               = render_pass;
    render_pass_begin_info.renderArea.offset.x      = 0;
    render_pass_begin_info.renderArea.offset.y      = 0;
    render_pass_begin_info.renderArea.extent.width  = width;
    render_pass_begin_info.renderArea.extent.height = height;
    render_pass_begin_info.clearValueCount          = clear_values.size();
    render_pass_begin_info.pClearValues             = clear_values.data();

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
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.color);

        // Set viewport dynamically
        VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        // Set scissor dynamically
        VkRect2D scissor = vkb::initializers::rect2D(width, height, 0, 0);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // Toggle color_write_enable for each attachment dynamically
        std::array<VkBool32, 3> color_write_enables = {r_bit_enabled, g_bit_enabled, b_bit_enabled};
        vkCmdSetColorWriteEnableEXT(cmd, static_cast<uint32_t>(color_write_enables.size()), color_write_enables.data());

        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layouts.composition, 0, 1, &descriptor_sets.composition, 0, NULL);

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
    if (drawer.header("Background color"))
    {
        if (drawer.slider_float("Red", &background_r_value, 0.0f, 1.0f))
        {
            build_command_buffers();
        }
        if (drawer.slider_float("Green", &background_g_value, 0.0f, 1.0f))
        {
            build_command_buffers();
        }
        if (drawer.slider_float("Blue", &background_b_value, 0.0f, 1.0f))
        {
            build_command_buffers();
        }
    }

    if (drawer.header("Enabled attachment"))
    {
        if (drawer.checkbox("Red bit", &r_bit_enabled))
        {
            build_command_buffers();
        }
        if (drawer.checkbox("Green bit", &g_bit_enabled))
        {
            build_command_buffers();
        }
        if (drawer.checkbox("Blue bit", &b_bit_enabled))
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

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

/**
 * @brief Color generation toggle using VK_EXT_color_write_enable
 */
class ColorWriteEnable : public ApiVulkanSample
{
  public:
    bool write_enabled = true;

    ColorWriteEnable();
    virtual ~ColorWriteEnable();

    // Create pipeline
    void prepare_pipelines();

    // Override basic framework functionality
    void build_command_buffers() override;
    void render(float delta_time) override;
    bool prepare(vkb::Platform &platform) override;
    void on_update_ui_overlay(vkb::Drawer &drawer) override;

  private:
    // Sample specific data
    VkPipeline       triangle_pipeline{};
    VkPipelineLayout triangle_pipeline_layout{};
};

std::unique_ptr<vkb::Application> create_color_write_enable();

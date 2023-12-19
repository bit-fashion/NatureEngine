/* ************************************************************************
 *
 * Copyright (C) 2022 Vincent Luo All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, e1ither express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ************************************************************************/

/* Creates on 2022/9/14. */

/*
  ===================================
    @author bit-fashion
  ===================================
*/
#include "VulkanContext.h"
#include "Window/Window.h"
#include "VulkanUtils.h"

VulkanContext::VulkanContext(Window *window) : m_Window(window) {
    InitVulkanDriverContext();
}

VulkanContext::~VulkanContext() {
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, VulkanUtils::Allocator);
    FreeCommandBuffer(std::size(m_CommandBuffers), std::data(m_CommandBuffers));
    vkDestroyCommandPool(m_Device, m_CommandPool, VulkanUtils::Allocator);
    vkDestroySemaphore(m_Device, m_SwapchainContext.renderFinishedSemaphore, VulkanUtils::Allocator);
    vkDestroySemaphore(m_Device, m_SwapchainContext.imageAvailableSemaphore, VulkanUtils::Allocator);
    DestroySwapchainContext(&m_SwapchainContext);
    vkDestroyDevice(m_Device, VulkanUtils::Allocator);
    vkDestroySurfaceKHR(m_Instance, m_SurfaceKHR, VulkanUtils::Allocator);
    vkDestroyInstance(m_Instance, VulkanUtils::Allocator);
}

void VulkanContext::_CreateSwapcahinAboutComponents(VkSwapchainContextKHR *pSwapchainContext) {
    VkSwapchainCreateInfoKHR swapchainCreateInfoKHR = {};
    swapchainCreateInfoKHR.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfoKHR.surface = m_SwapchainContext.surface;
    swapchainCreateInfoKHR.minImageCount = m_SwapchainContext.minImageCount;
    swapchainCreateInfoKHR.imageFormat = m_SwapchainContext.surfaceFormat.format;
    swapchainCreateInfoKHR.imageColorSpace = m_SwapchainContext.surfaceFormat.colorSpace;
    swapchainCreateInfoKHR.imageExtent = { m_SwapchainContext.width, m_SwapchainContext.height };
    swapchainCreateInfoKHR.imageArrayLayers = 1;
    swapchainCreateInfoKHR.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfoKHR.preTransform = m_SwapchainContext.capabilities.currentTransform;
    swapchainCreateInfoKHR.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfoKHR.presentMode = m_SwapchainContext.presentMode;
    swapchainCreateInfoKHR.clipped = VK_TRUE;
    swapchainCreateInfoKHR.oldSwapchain = null;

    vkCreateSwapchainKHR(m_Device, &swapchainCreateInfoKHR, VulkanUtils::Allocator,
                         &m_SwapchainContext.swapchain);

    vkGetSwapchainImagesKHR(m_Device, pSwapchainContext->swapchain, &pSwapchainContext->minImageCount, null);
    pSwapchainContext->images.resize(pSwapchainContext->minImageCount);
    vkGetSwapchainImagesKHR(m_Device, pSwapchainContext->swapchain, &pSwapchainContext->minImageCount, std::data(pSwapchainContext->images));

    /* create swapcahin image view and framebuffer */
    pSwapchainContext->imageViews.resize(pSwapchainContext->minImageCount);
    pSwapchainContext->framebuffers.resize(pSwapchainContext->minImageCount);
    for (uint32_t i = 0; i < pSwapchainContext->minImageCount; i++) {
        /* view */
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = pSwapchainContext->images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = pSwapchainContext->surfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(m_Device, &imageViewCreateInfo, VulkanUtils::Allocator, &pSwapchainContext->imageViews[i]);

        /* framebuffer */
        VkImageView attachments[] = { pSwapchainContext->imageViews[i] };
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = pSwapchainContext->renderpass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = pSwapchainContext->width;
        framebufferCreateInfo.height = pSwapchainContext->height;
        framebufferCreateInfo.layers = 1;

        vkCreateFramebuffer(m_Device, &framebufferCreateInfo, VulkanUtils::Allocator, &pSwapchainContext->framebuffers[i]);
    }
}

void VulkanContext::_CreateRenderpass(VkSwapchainContextKHR *pSwapchainContext) {
    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.format = pSwapchainContext->surfaceFormat.format;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    vkCreateRenderPass(m_Device, &renderPassCreateInfo, VulkanUtils::Allocator, &pSwapchainContext->renderpass);
}

void VulkanContext::_ConfigurationSwapchainContext(VkSwapchainContextKHR *pSwapchainContext) {
    VulkanUtils::ConfigurationVulkanSwapchainContextDetail(m_PhysicalDevice, m_SurfaceKHR, m_Window,
                                                       &m_SwapchainContext);
}

void VulkanContext::_ConfigurationWindowResizeableEventCallback() {
    m_Window->AddWindowResizeableCallback([](Window *window, int width, int height){
        VulkanContext *context = (VulkanContext *) window->GetWindowUserPointer("VulkanContext");
        context->RecreateSwapchainContext(&context->m_SwapchainContext, width, height);
    });
}

void VulkanContext::DeviceWaitIdle() {
    vkDeviceWaitIdle(m_Device);
}

void VulkanContext::CopyBuffer(VkDeviceBuffer dest, VkDeviceBuffer src, VkDeviceSize size) {
    VkCommandBuffer oneTimeCommandBuffer;
    BeginOnceTimeCommandBufferSubmit(&oneTimeCommandBuffer);
    {
        /* copy buffer */
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(oneTimeCommandBuffer, src.buffer, dest.buffer, 1, &copyRegion);
    }
    EndOnceTimeCommandBufferSubmit();
}

void VulkanContext::MapMemory(VkDeviceBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData) {
    vkMapMemory(m_Device, buffer.memory, offset, size, flags, ppData);
}

void VulkanContext::UnmapMemory(VkDeviceBuffer buffer) {
    vkUnmapMemory(m_Device, buffer.memory);
}

void VulkanContext::BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags) {
    /* start command buffers record. */
    vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = usageFlags;
    commandBufferBeginInfo.pInheritanceInfo = null; // Optional
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
}

void VulkanContext::EndCommandBuffer(VkCommandBuffer commandBuffer) {
    /* end command buffer record. */
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("failed to record command buffer!");
}

void VulkanContext::SyncSubmitQueueWithSubmitInfo(uint32_t commandBufferCount, VkCommandBuffer *pCommandBuffers,
                                                 uint32_t waitSemaphoreCount, VkSemaphore *pWaitSemaphores,
                                                 uint32_t signalSemaphoreCount, VkSemaphore *pSignalSemaphores,
                                                 VkPipelineStageFlags *pWaitDstStageMask) {
    /* submit command buffer */
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = waitSemaphoreCount;
    submitInfo.pWaitSemaphores = pWaitSemaphores;
    submitInfo.pWaitDstStageMask = pWaitDstStageMask;
    submitInfo.commandBufferCount = commandBufferCount;
    submitInfo.pCommandBuffers = pCommandBuffers;

    submitInfo.signalSemaphoreCount = signalSemaphoreCount;
    submitInfo.pSignalSemaphores = pSignalSemaphores;

    if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        throw std::runtime_error("failed to submit draw command buffer!");

    vkQueueWaitIdle(m_GraphicsQueue);
}

void VulkanContext::BeginOnceTimeCommandBufferSubmit(VkCommandBuffer *pCommandBuffer) {
    AllocateCommandBuffer(1, &m_SingleTimeCommandBuffer);
    *pCommandBuffer = m_SingleTimeCommandBuffer;
    /* begin */
    BeginCommandBuffer(m_SingleTimeCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void VulkanContext::EndOnceTimeCommandBufferSubmit() {
    EndCommandBuffer(m_SingleTimeCommandBuffer);
    /* submit */
    SyncSubmitQueueWithSubmitInfo(1, &m_SingleTimeCommandBuffer, 0, NULL, 0, NULL, NULL);
    FreeCommandBuffer(1, &m_SingleTimeCommandBuffer);
}

void VulkanContext::BeginRecordCommandBuffer() {
    BeginCommandBuffer(m_FrameContext.commandBuffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
}

void VulkanContext::EndRecordCommandBuffer() {
    EndCommandBuffer(m_FrameContext.commandBuffer);
}

void VulkanContext::BeginRenderPass(VkRenderPass renderPass) {
    /* start render pass. */
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = m_FrameContext.framebuffer;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = { m_SwapchainContext.width, m_SwapchainContext.height };

    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(m_FrameContext.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanContext::EndRenderPass() {
    /* end render pass */
    vkCmdEndRenderPass(m_FrameContext.commandBuffer);
}

void VulkanContext::QueueWaitIdle(VkQueue queue) {
    vkQueueWaitIdle(queue);
}

void VulkanContext::BeginRender(VkFrameContext **ppFrameContext) {
    uint32_t index;
    vkAcquireNextImageKHR(m_Device, m_SwapchainContext.swapchain, std::numeric_limits<uint64_t>::max(),
                          m_SwapchainContext.imageAvailableSemaphore, null, &index);
    m_FrameContext.index = index;
    m_FrameContext.framebuffer = m_SwapchainContext.framebuffers[m_FrameContext.index];
    m_FrameContext.commandBuffer = m_CommandBuffers[m_FrameContext.index];

    if (ppFrameContext != null)
        GetFrameContext(ppFrameContext);

    BeginRecordCommandBuffer();
    BeginRenderPass(m_SwapchainContext.renderpass);
}

void VulkanContext::EndRender() {
    EndRenderPass();
    EndRecordCommandBuffer();
    /* final submit */
    VkSemaphore waitSemaphores[] = { m_SwapchainContext.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = { m_SwapchainContext.renderFinishedSemaphore };

    SyncSubmitQueueWithSubmitInfo(1, &m_FrameContext.commandBuffer,
                                  1, waitSemaphores,
                                  1, signalSemaphores,
                                  waitStages);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_SwapchainContext.swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_FrameContext.index;
    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    QueueWaitIdle(m_PresentQueue);
}

void VulkanContext::BindRenderPipeline(VkRenderPipeline &pipeline) {
    vkCmdBindPipeline(m_FrameContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    // 动态视口
    float w = m_SwapchainContext.width;
    float h = m_SwapchainContext.height;
    VkViewport viewport = { 0.0f, 0.0f, w, h, 0.0f, 1.0f };
    vkCmdSetViewport(m_FrameContext.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = { m_SwapchainContext.width, m_SwapchainContext.height };
    vkCmdSetScissor(m_FrameContext.commandBuffer, 0, 1, &scissor);
}

void VulkanContext::BindDescriptorSets(VkRenderPipeline &pipeline, uint32_t count, VkDescriptorSet *pDescriptorSets) {
    vkCmdBindDescriptorSets(m_FrameContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.pipelineLayout, 0, count, pDescriptorSets, 0, null);
}

void VulkanContext::WriteDescriptorSet(VkDeviceBuffer &buffer, VkTexture2D &texture, VkDescriptorSet descriptorSet) {
    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = buffer.size;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[0].pImageInfo = nullptr; // Optional
    descriptorWrites[0].pTexelBufferView = nullptr; // Optional

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.imageView;
    imageInfo.sampler = texture.sampler;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = nullptr;
    descriptorWrites[1].pImageInfo = &imageInfo;
    descriptorWrites[1].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_Device, std::size(descriptorWrites),
                           std::data(descriptorWrites), 0, nullptr);
}

void VulkanContext::DrawIndexed(uint32_t indexCount) {
    vkCmdDrawIndexed(m_FrameContext.commandBuffer, indexCount, 1, 0, 0, 0);
}

void VulkanContext::AllocateVertexBuffer(VkDeviceSize size, const Vertex *pVertices, VkDeviceBuffer *pVertexBuffer) {
    VkDeviceBuffer stagingBuffer;
    AllocateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);
    void *data;
    MapMemory(stagingBuffer, 0, stagingBuffer.size, 0, &data);
    memcpy(data, pVertices, static_cast<VkDeviceSize>(stagingBuffer.size));
    UnmapMemory(stagingBuffer);
    /* vertex buffer */
    AllocateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                   VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, pVertexBuffer);
    CopyBuffer(*pVertexBuffer, stagingBuffer, size);
    FreeBuffer(stagingBuffer);
}

void VulkanContext::AllocateIndexBuffer(VkDeviceSize size, const uint32_t *pIndices, VkDeviceBuffer *pIndexBuffer) {
    VkDeviceBuffer stagingBuffer;
    AllocateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);
    void *data;
    MapMemory(stagingBuffer, 0, stagingBuffer.size, 0, &data);
    memcpy(data, pIndices, static_cast<VkDeviceSize>(stagingBuffer.size));
    UnmapMemory(stagingBuffer);
    /* vertex buffer */
    AllocateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, pIndexBuffer);
    CopyBuffer(*pIndexBuffer, stagingBuffer, size);
    FreeBuffer(stagingBuffer);
}

void VulkanContext::TransitionTextureLayout(VkTexture2D *texture, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer;
    BeginOnceTimeCommandBufferSubmit(&commandBuffer);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = texture->layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0; // TODO
    barrier.dstAccessMask = 0; // TODO

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (texture->layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (texture->layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    EndOnceTimeCommandBufferSubmit();

    texture->layout = newLayout;
}

void VulkanContext::CopyTextureBuffer(VkDeviceBuffer &buffer, VkTexture2D &texture, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer;
    BeginOnceTimeCommandBufferSubmit(&commandBuffer);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
            width,
            height,
            1
    };

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer.buffer,
            texture.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
    );

    EndOnceTimeCommandBufferSubmit();
}

void VulkanContext::CreateTexture2D(const String &path, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                    VkMemoryPropertyFlags properties, VkTexture2D *pTexture2D) {
    /* load image. */
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(_chars(path), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels)
        throw std::runtime_error("failed to load texture image!");

    VkDeviceBuffer stagingBuffer;
    AllocateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer);

    void *data;
    MapMemory(stagingBuffer, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    UnmapMemory(stagingBuffer);

    stbi_image_free(pixels);

    /* Create image */
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageCreateInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.flags = 0; // Optional

    pTexture2D->format = imageCreateInfo.format;
    pTexture2D->layout = imageCreateInfo.initialLayout;

    vkCreateImage(m_Device, &imageCreateInfo, VulkanUtils::Allocator, &pTexture2D->image);

    /* bind memory */
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(m_Device, pTexture2D->image, &requirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = VulkanUtils::FindMemoryType(requirements.memoryTypeBits, m_PhysicalDevice, properties);

    vkAllocateMemory(m_Device, &allocInfo, VulkanUtils::Allocator, &pTexture2D->memory);

    vkBindImageMemory(m_Device, pTexture2D->image, pTexture2D->memory, 0);

    /* create image view */
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = pTexture2D->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(m_Device, &viewInfo, nullptr, &pTexture2D->imageView);

    /* create texture  sampler */
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vkCreateSampler(m_Device, &samplerInfo, VK_NULL_HANDLE, &pTexture2D->sampler);

    /* copy buffer to image */
    TransitionTextureLayout(pTexture2D, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyTextureBuffer(stagingBuffer, *pTexture2D, texWidth, texHeight);
    TransitionTextureLayout(pTexture2D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    FreeBuffer(stagingBuffer);
}

void VulkanContext::CreateSemaphore(VkSemaphore *pSemaphore) {
    /** Create semaphores. */
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(m_Device, &semaphoreCreateInfo, VulkanUtils::Allocator, pSemaphore);
}

void VulkanContext::CreateDescriptorSetLayout(std::vector<VkDescriptorSetLayoutBinding> &bindings, VkDescriptorSetLayoutCreateFlags flags,
                                             VkDescriptorSetLayout *pDescriptorSetLayout) {
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.flags = flags;
    descriptorSetLayoutCreateInfo.bindingCount = std::size(bindings);
    descriptorSetLayoutCreateInfo.pBindings = std::data(bindings);
    vkCreateDescriptorSetLayout(m_Device, &descriptorSetLayoutCreateInfo, VulkanUtils::Allocator, pDescriptorSetLayout);
}

void VulkanContext::AllocateDescriptorSet(Vector<VkDescriptorSetLayout> &layouts, VkDescriptorSet *pDescriptorSet) {
    /** Allocate descriptor set */
    VkDescriptorSetAllocateInfo descriptorAllocateInfo = {};
    descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocateInfo.descriptorPool = m_DescriptorPool;
    descriptorAllocateInfo.descriptorSetCount = std::size(layouts);
    descriptorAllocateInfo.pSetLayouts = std::data(layouts);
    vkAllocateDescriptorSets(m_Device, &descriptorAllocateInfo, pDescriptorSet);
}

void VulkanContext::CreateRenderPipeline(const String &shaderfolder, const String &shadername, VkDescriptorSetLayout descriptorSetLayout,
                                           VkRenderPipeline *pDriverGraphicsPipeline) {
    /** Create shader of vertex & fragment module. */
    VkShaderModule vertexShaderModule =
            VulkanUtils::LoadShaderModule(m_Device, shaderfolder, shadername, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderModule fragmentShaderModule =
            VulkanUtils::LoadShaderModule(m_Device, shaderfolder, shadername, VK_SHADER_STAGE_FRAGMENT_BIT);

    /** Create pipeline phase of vertex and fragment shader */
    VkPipelineShaderStageCreateInfo pipelineVertexShaderStageCreateInfo = {};
    pipelineVertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineVertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineVertexShaderStageCreateInfo.module = vertexShaderModule;
    pipelineVertexShaderStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo pipelineFragmentStageCreateInfo = {};
    pipelineFragmentStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineFragmentStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineFragmentStageCreateInfo.module = fragmentShaderModule;
    pipelineFragmentStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = { pipelineVertexShaderStageCreateInfo, pipelineFragmentStageCreateInfo };

    /* pipeline features */
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription vertexInputBindingDescription = VulkanUtils::GetVertexInputBindingDescription();
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;

    auto vertexInputAttributeDescriptions = VulkanUtils::GetVertexInputAttributeDescriptionArray();
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = std::size(vertexInputAttributeDescriptions);
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = std::data(vertexInputAttributeDescriptions);

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssembly = {};
    pipelineInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_SwapchainContext.width;
    viewport.height = (float) m_SwapchainContext.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = { m_SwapchainContext.width, m_SwapchainContext.height };

    /* 视口裁剪 */
    VkPipelineViewportStateCreateInfo pipelineViewportStateCrateInfo = {};
    pipelineViewportStateCrateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCrateInfo.viewportCount = 1;
    pipelineViewportStateCrateInfo.pViewports = &viewport;
    pipelineViewportStateCrateInfo.scissorCount = 1;
    pipelineViewportStateCrateInfo.pScissors = &scissor;

    /* 光栅化阶段 */
    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f; // Optional
    pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f; // Optional
    pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f; // Optional

    /* 多重采样 */
    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f; // Optional
    pipelineMultisampleStateCreateInfo.pSampleMask = nullptr; // Optional
    pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optional

    /* 颜色混合 */
    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
    pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    pipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    /* 帧缓冲 */
    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    pipelineColorBlendStateCreateInfo.attachmentCount = 1;
    pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
    pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
    pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
    pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
    pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

    /* 动态修改 */
    Vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.dynamicStateCount = std::size(dynamicStates);
    pipelineDynamicStateCreateInfo.pDynamicStates = std::data(dynamicStates);

    /* 管道布局 */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, VulkanUtils::Allocator, &pDriverGraphicsPipeline->pipelineLayout);

    /** Create graphics pipeline in vulkan. */
    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos;
    graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssembly;
    graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCrateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr; // Optional
    graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo; // Optional
    graphicsPipelineCreateInfo.layout = pDriverGraphicsPipeline->pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = m_SwapchainContext.renderpass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    graphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional

    vkCreateGraphicsPipelines(m_Device, null, 1, &graphicsPipelineCreateInfo,
                              VulkanUtils::Allocator, &pDriverGraphicsPipeline->pipeline);

    /* 销毁着色器模块 */
    vkDestroyShaderModule(m_Device, vertexShaderModule, VulkanUtils::Allocator);
    vkDestroyShaderModule(m_Device, fragmentShaderModule, VulkanUtils::Allocator);
}

void VulkanContext::AllocateCommandBuffer(uint32_t count, VkCommandBuffer *pCommandBuffer) {
    /** Allocate command buffer. */
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = m_CommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = (uint32_t) count;

    vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, pCommandBuffer);
}

void VulkanContext::AllocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                   VkDeviceBuffer *buffer) {
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    buffer->size = bufferCreateInfo.size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m_Device, &bufferCreateInfo, VulkanUtils::Allocator, &buffer->buffer);

    /** Query memory requirements. */
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_Device, buffer->buffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = VulkanUtils::FindMemoryType(memoryRequirements.memoryTypeBits, m_PhysicalDevice, properties);

    vkAllocateMemory(m_Device, &memoryAllocInfo, VulkanUtils::Allocator, &buffer->memory);
    vkBindBufferMemory(m_Device, buffer->buffer, buffer->memory, 0);
}

void VulkanContext::RecreateSwapchainContext(VkSwapchainContextKHR *pSwapchainContext, uint32_t width, uint32_t height) {
    if (width <= 0 || height <= 0)
        return;
    DeviceWaitIdle();
    DestroySwapchainContext(pSwapchainContext);
    CreateSwapchainContext(pSwapchainContext);
}

void VulkanContext::CreateSwapchainContext(VkSwapchainContextKHR *pSwapchainContext) {
    _ConfigurationSwapchainContext(pSwapchainContext);
    _CreateRenderpass(pSwapchainContext);
    _CreateSwapcahinAboutComponents(pSwapchainContext);
}

void VulkanContext::InitVulkanDriverContext() {
    m_Window->PutWindowUserPointer("VulkanContext", this);
    _ConfigurationWindowResizeableEventCallback();

    /* init stages */
    _InitVulkanContextInstance();
    _InitVulkanContextSurface();
    _InitVulkanContextDevice();
    _InitVulkanContextQueue();
    _InitVulkanContextCommandPool();
    _InitVulkanContextSwapchain();
    _InitVulkanContextCommandBuffers();
    _InitVulkanContextDescriptorPool();

    m_RenderContext.Instance = m_Instance;
    m_RenderContext.Surface = m_SurfaceKHR;
    m_RenderContext.PhysicalDevice = m_PhysicalDevice;
    m_RenderContext.Device = m_Device;
    m_RenderContext.GraphicsQueue = m_GraphicsQueue;
    m_RenderContext.GraphicsQueueFamily = m_GraphicsQueueFamily;
    m_RenderContext.Swapchain = m_SwapchainContext.swapchain;
    m_RenderContext.RenderPass = m_SwapchainContext.renderpass;
    m_RenderContext.CommandPool = m_CommandPool;
    m_RenderContext.DescriptorPool = m_DescriptorPool;
    m_RenderContext.MinImageCount = m_SwapchainContext.minImageCount;
    m_RenderContext.FrameContext = &m_FrameContext;
}

void VulkanContext::_InitVulkanContextInstance() {
    /* Create vulkan instance. */
    struct VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = VK_VERSION_1_3;
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = SPORTS_ENGINE_NAME;
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = SPORTS_ENGINE_NAME;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    static Vector<const char *> requiredEnableExtensionsForInstance;
    VulkanUtils::GetVulkanInstanceRequiredExtensions(requiredEnableExtensionsForInstance);
    instanceCreateInfo.enabledExtensionCount = std::size(requiredEnableExtensionsForInstance);
    instanceCreateInfo.ppEnabledExtensionNames = std::data(requiredEnableExtensionsForInstance);

    static Vector<const char *> requiredEnableLayersForInstance;
    VulkanUtils::GetVulkanInstanceRequiredLayers(requiredEnableLayersForInstance);
    instanceCreateInfo.enabledLayerCount = std::size(requiredEnableLayersForInstance);
    instanceCreateInfo.ppEnabledLayerNames = std::data(requiredEnableLayersForInstance);
    vkCreateInstance(&instanceCreateInfo, VulkanUtils::Allocator, &m_Instance);
}

void VulkanContext::_InitVulkanContextSurface() {
    /* Create window surface */
#ifdef _glfw3_h_
    VulkanUtils::CreateWindowSurfaceKHR(m_Instance, m_Window->GetWindowPointer(), &m_SurfaceKHR);
#endif
}

void VulkanContext::_InitVulkanContextDevice() {
    /* Create vulkan device. */
    VulkanUtils::GetVulkanMostPreferredPhysicalDevice(m_Instance, &m_PhysicalDevice, &m_PhysicalDeviceProperties,
                                                      &m_PhysicalDeviceFeature);
    Vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
    VulkanUtils::QueueFamilyIndices queueFamilyIndices =
            VulkanUtils::GetVulkanDeviceCreateRequiredQueueFamilyAndQueueCreateInfo(m_PhysicalDevice, m_SurfaceKHR, deviceQueueCreateInfos);

    m_GraphicsQueueFamily = queueFamilyIndices.graphicsQueueFamily;
    m_PresentQueueFamily = queueFamilyIndices.presentQueueFamily;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    static VkPhysicalDeviceFeatures features = {};
    deviceCreateInfo.pEnabledFeatures = &features;

    static Vector<const char *> requiredEnableExtensions;
    VulkanUtils::GetVulkanDeviceRequiredExtensions(requiredEnableExtensions);
    deviceCreateInfo.enabledExtensionCount = std::size(requiredEnableExtensions);
    deviceCreateInfo.ppEnabledExtensionNames = std::data(requiredEnableExtensions);

    static Vector<const char *> requiredEnableLayers;
    VulkanUtils::GetVulkanDeviceRequiredLayers(requiredEnableLayers);
    deviceCreateInfo.enabledLayerCount = std::size(requiredEnableLayers);
    deviceCreateInfo.ppEnabledLayerNames = std::data(requiredEnableLayers);

    deviceCreateInfo.queueCreateInfoCount = std::size(deviceQueueCreateInfos);
    deviceCreateInfo.pQueueCreateInfos = std::data(deviceQueueCreateInfos);

    vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, VulkanUtils::Allocator, &m_Device);
}

void VulkanContext::_InitVulkanContextQueue() {
    /* get queue */
    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);
}

void VulkanContext::_InitVulkanContextCommandPool() {
    /* Create command pool. */
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(m_Device, &commandPoolCreateInfo, VulkanUtils::Allocator, &m_CommandPool);
}

void VulkanContext::_InitVulkanContextSwapchain() {
    /* Create swapchain */
    CreateSwapchainContext(&m_SwapchainContext);
    CreateSemaphore(&m_SwapchainContext.imageAvailableSemaphore);
    CreateSemaphore(&m_SwapchainContext.renderFinishedSemaphore);
}

void VulkanContext::_InitVulkanContextCommandBuffers() {
    /* allocate */
    m_CommandBuffers.resize(m_SwapchainContext.minImageCount);
    AllocateCommandBuffer(std::size(m_CommandBuffers), std::data(m_CommandBuffers));
}

void VulkanContext::_InitVulkanContextDescriptorPool() {
    /** Create descriptor set pool */
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_SAMPLER,                1024},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1024},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1024},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1024},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1024}
    };

    VkDescriptorPoolCreateInfo descriptorPoolCrateInfo = {};
    descriptorPoolCrateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCrateInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    descriptorPoolCrateInfo.pPoolSizes = std::data(poolSizes);
    descriptorPoolCrateInfo.maxSets = 1024 * std::size(poolSizes);
    descriptorPoolCrateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    vkCreateDescriptorPool(m_Device, &descriptorPoolCrateInfo, VulkanUtils::Allocator, &m_DescriptorPool);
}

void VulkanContext::DestroyTexture2D(VkTexture2D &texture) {
    vkDestroySampler(m_Device, texture.sampler, VulkanUtils::Allocator);
    vkDestroyImageView(m_Device, texture.imageView, VulkanUtils::Allocator);
    vkDestroyImage(m_Device, texture.image, VulkanUtils::Allocator);
    vkFreeMemory(m_Device, texture.memory, VulkanUtils::Allocator);
}

void VulkanContext::FreeDescriptorSets(uint32_t count, VkDescriptorSet *pDescriptorSet) {
    vkFreeDescriptorSets(m_Device, m_DescriptorPool, count, pDescriptorSet);
}

void VulkanContext::DestroyDescriptorSetLayout(VkDescriptorSetLayout &descriptorSetLayout) {
    vkDestroyDescriptorSetLayout(m_Device, descriptorSetLayout, VulkanUtils::Allocator);
}

void VulkanContext::DestroyRenderPipeline(VkRenderPipeline &pipeline) {
    vkDestroyPipeline(m_Device, pipeline.pipeline, VulkanUtils::Allocator);
    vkDestroyPipelineLayout(m_Device, pipeline.pipelineLayout, VulkanUtils::Allocator);
}

void VulkanContext::FreeCommandBuffer(uint32_t count, VkCommandBuffer *pCommandBuffer) {
    vkFreeCommandBuffers(m_Device, m_CommandPool, count, pCommandBuffer);
}

void VulkanContext::FreeBuffer(VkDeviceBuffer &buffer) {
    vkFreeMemory(m_Device, buffer.memory, VulkanUtils::Allocator);
    vkDestroyBuffer(m_Device, buffer.buffer, VulkanUtils::Allocator);
}

void VulkanContext::DestroySwapchainContext(VkSwapchainContextKHR *pSwapchainContext) {
    vkDestroyRenderPass(m_Device, pSwapchainContext->renderpass, VulkanUtils::Allocator);
    for (int i = 0; i < pSwapchainContext->minImageCount; i++) {
        vkDestroyImageView(m_Device, pSwapchainContext->imageViews[i], VulkanUtils::Allocator);
        vkDestroyFramebuffer(m_Device, pSwapchainContext->framebuffers[i], VulkanUtils::Allocator);
    }
    vkDestroySwapchainKHR(m_Device, pSwapchainContext->swapchain, VulkanUtils::Allocator);
}
#include "GpuContext.h"
#include <fstream>
#include <cstring>
#include <cmath>
#include <vector>

static std::vector<uint8_t> readFile(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

bool GpuContext::init(SDL_Window* win) {
    window = win;
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "direct3d12");
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    device = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);
    if (!device) return false;
    SDL_ClaimWindowForGPUDevice(device, window);

    auto vsCode = readFile("shaders/cube.vert.dxil");
    auto fsCode = readFile("shaders/cube.frag.dxil");
    if (vsCode.empty() || fsCode.empty()) { SDL_Log("Shader not found!"); return false; }

    SDL_GPUShaderCreateInfo vsInfo{};
    vsInfo.code = vsCode.data(); vsInfo.code_size = vsCode.size();
    vsInfo.entrypoint = "main"; vsInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
    vsInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX; vsInfo.num_uniform_buffers = 1;
    SDL_GPUShader* vs = SDL_CreateGPUShader(device, &vsInfo);

    SDL_GPUShaderCreateInfo fsInfo{};
    fsInfo.code = fsCode.data(); fsInfo.code_size = fsCode.size();
    fsInfo.entrypoint = "main"; fsInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
    fsInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    SDL_GPUShader* fs = SDL_CreateGPUShader(device, &fsInfo);

    // 顶点槽 0：3 pos + 2 edgeMask = 5 floats
    SDL_GPUVertexBufferDescription vbDesc[2]{};
    vbDesc[0].slot = 0; vbDesc[0].pitch = sizeof(float) * 5; vbDesc[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vbDesc[1].slot = 1; vbDesc[1].pitch = sizeof(CubeInstance); vbDesc[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;

    SDL_GPUVertexAttribute attrs[6]{};
    attrs[0] = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0};                 // pos
    attrs[1] = {5, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(float)*3};   // edgeMask

    attrs[2] = {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0};                 // axis
    attrs[3] = {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float)*3};   // center
    attrs[4] = {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,  sizeof(float)*6};   // angle
    attrs[5] = {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, sizeof(float)*7};   // color

    SDL_GPUGraphicsPipelineCreateInfo pInfo{};
    pInfo.vertex_shader = vs; pInfo.fragment_shader = fs;
    pInfo.vertex_input_state.vertex_buffer_descriptions = vbDesc;
    pInfo.vertex_input_state.num_vertex_buffers = 2;
    pInfo.vertex_input_state.vertex_attributes = attrs;
    pInfo.vertex_input_state.num_vertex_attributes = 6;
    pInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE; // 保持双面渲染
    pInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pInfo.depth_stencil_state.enable_depth_test = true;
    pInfo.depth_stencil_state.enable_depth_write = true;
    pInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    pInfo.target_info.color_target_descriptions = &colorTarget;
    pInfo.target_info.num_color_targets = 1;
    pInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    pInfo.target_info.has_depth_stencil_target = true;

    pipeline = SDL_CreateGPUGraphicsPipeline(device, &pInfo);
    SDL_ReleaseGPUShader(device, vs); SDL_ReleaseGPUShader(device, fs);

    // 静态几何体：使用独立顶点，每个面四个角的遮罩固定
    float s = 0.5f;
    float verts[8][3] = {{-s,-s,-s},{s,-s,-s},{s,s,-s},{-s,s,-s},{-s,-s,s},{s,-s,s},{s,s,s},{-s,s,s}};
    int faces[6][4] = {
        {4,5,6,7}, // 前
        {1,0,3,2}, // 后
        {0,4,7,3}, // 左
        {5,1,2,6}, // 右
        {3,7,6,2}, // 上
        {0,1,5,4}  // 下
    };
    
    // 标准遮罩布局：(左下, 右下, 右上, 左上)
    float faceMasks[4][2] = {
        {0.0f, 1.0f}, // 左下
        {1.0f, 1.0f}, // 右下
        {1.0f, 0.0f}, // 右上
        {0.0f, 0.0f}  // 左上
    };

    std::vector<float> vData;
    for(int f=0; f<6; f++) {
        // 每个面由 2 个三角形组成 (0-1-2, 0-2-3)
        int indices[6] = {0, 1, 2, 0, 2, 3};
        for(int i=0; i<6; i++) {
            int cornerIdx = indices[i];
            int vi = faces[f][cornerIdx];
            
            // 插入坐标 (3 floats)
            vData.insert(vData.end(), {verts[vi][0], verts[vi][1], verts[vi][2]});
            // 插入遮罩 (2 floats)
            vData.insert(vData.end(), {faceMasks[cornerIdx][0], faceMasks[cornerIdx][1]});
        }
    }
    
    // 索引数组现在直接就是 0 ~ 35
    std::vector<uint16_t> iData;
    for(uint16_t i=0; i<36; i++) iData.push_back(i);

    SDL_GPUBufferCreateInfo bInfo{};
    bInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX; bInfo.size = (Uint32)(vData.size() * sizeof(float));
    vertexBuffer = SDL_CreateGPUBuffer(device, &bInfo);
    bInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX; bInfo.size = (Uint32)(iData.size() * sizeof(uint16_t));
    indexBuffer = SDL_CreateGPUBuffer(device, &bInfo);

    bInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX; bInfo.size = sizeof(CubeInstance) * 1024;
    instanceBuffer = SDL_CreateGPUBuffer(device, &bInfo);

    SDL_GPUTransferBufferCreateInfo tbInfo{};
    tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; tbInfo.size = sizeof(CubeInstance) * 1024;
    transferBuffer = SDL_CreateGPUTransferBuffer(device, &tbInfo);

    // 上传静态数据
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    
    SDL_GPUTransferBufferCreateInfo tbStaticInfo{};
    tbStaticInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbStaticInfo.size = (Uint32)(vData.size() * sizeof(float) + iData.size() * sizeof(uint16_t));
    SDL_GPUTransferBuffer* staticTb = SDL_CreateGPUTransferBuffer(device, &tbStaticInfo);

    void* map = SDL_MapGPUTransferBuffer(device, staticTb, false);
    memcpy(map, vData.data(), vData.size()*sizeof(float));
    memcpy((char*)map + vData.size()*sizeof(float), iData.data(), iData.size()*sizeof(uint16_t));
    SDL_UnmapGPUTransferBuffer(device, staticTb);

    SDL_GPUTransferBufferLocation src{staticTb, 0};
    SDL_GPUBufferRegion dst{vertexBuffer, 0, (Uint32)(vData.size()*sizeof(float))};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    src.offset = (Uint32)(vData.size()*sizeof(float));
    dst = {indexBuffer, 0, (Uint32)(iData.size()*sizeof(uint16_t))};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staticTb);
    return true;
}

void GpuContext::draw(const std::vector<CubeInstance>& instances, const float viewProj[16], float scale) {
    if (instances.empty()) return;
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUTexture* swapchain = nullptr;
    SDL_AcquireGPUSwapchainTexture(cmd, window, &swapchain, nullptr, nullptr);
    if (!swapchain) { SDL_SubmitGPUCommandBuffer(cmd); return; }

    void* map = SDL_MapGPUTransferBuffer(device, transferBuffer, true);
    memcpy(map, instances.data(), instances.size() * sizeof(CubeInstance));
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src{transferBuffer, 0};
    SDL_GPUBufferRegion dst{instanceBuffer, 0, (Uint32)(instances.size() * sizeof(CubeInstance))};
    SDL_UploadToGPUBuffer(copy, &src, &dst, true);
    SDL_EndGPUCopyPass(copy);

    int w, h; SDL_GetWindowSizeInPixels(window, &w, &h);
    if (!depthTexture || (uint32_t)w != depthWidth || (uint32_t)h != depthHeight) {
        if (depthTexture) SDL_ReleaseGPUTexture(device, depthTexture);
        SDL_GPUTextureCreateInfo depthInfo{};
        depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthInfo.width = (Uint32)w;
        depthInfo.height = (Uint32)h;
        depthInfo.layer_count_or_depth = 1;
        depthInfo.num_levels = 1;
        depthTexture = SDL_CreateGPUTexture(device, &depthInfo);
        depthWidth = w; depthHeight = h;
    }

    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = swapchain;
    colorTarget.clear_color = {0.08f, 0.08f, 0.11f, 1.0f};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget{};
    depthTarget.texture = depthTexture;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_STORE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &depthTarget);
    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUBufferBinding vb[2] = {{vertexBuffer, 0}, {instanceBuffer, 0}};
    SDL_BindGPUVertexBuffers(pass, 0, vb, 2);
    SDL_GPUBufferBinding ib = {indexBuffer, 0};
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    struct Uniforms { float viewProj[16]; float scale; float pad[3]; } u{};
    memcpy(u.viewProj, viewProj, sizeof(u.viewProj));
    u.scale = scale;
    SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));

    SDL_DrawGPUIndexedPrimitives(pass, 36, (Uint32)instances.size(), 0, 0, 0);

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
}
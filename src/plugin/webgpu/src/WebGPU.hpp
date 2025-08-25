#pragma once

// --- Plugin ---
#include "PluginWebGPU.hpp"

// --- Component ---
#include "Mesh.hpp"

// --- Resource
#include "RenderGraph.hpp"

// --- Util ---
#include "CreateSprite.hpp"
#include "util/structs.hpp"
#include "Texture.hpp"
#include "UpdateLights.hpp"
#include "utils.hpp"
#include "util/webgpu.hpp"

// --- System ---
// Setup
#include "CreateInstance.hpp"
#include "CreateSurface.hpp"
#include "CreateAdapter.hpp"
#include "AdaptaterPrint.hpp"
#include "ReleaseInstance.hpp"
#include "RequestCapabilities.hpp"
#include "CreateDevice.hpp"
#include "CreateQueue.hpp"
#include "SetupQueueOnSubmittedWorkDone.hpp"
#include "ConfigureSurface.hpp"
#include "ReleaseAdapter.hpp"
#include "InspectDevice.hpp"
#include "GenerateDefaultTexture.hpp"
#include "InitDepthBuffer.hpp"
#include "InitializePipeline.hpp"
#include "Initialize2DPipeline.hpp"
#include "InitializeDeferredPipeline.hpp"
#include "InitializeGBufferPipeline.hpp"
#include "InitializeShadowPipeline.hpp"
#include "InitBuffers.hpp"
#include "InitGBufferBuffers.hpp"
#include "InitGBufferTextures.hpp"
#include "InitShadowTexture.hpp"
#include "Create2DPipelineBuffer.hpp"
#include "CreateBindingGroup.hpp"
#include "CreateBindingGroup2D.hpp"
#include "CreateBindingGroupDeferred.hpp"
#include "CreateBindingGroupGBuffer.hpp"
#include "CreateBindingGroupShadows.hpp"
#include "SetupResizableWindow.hpp"

// To GPU
#include "UpdateBuffers.hpp"
#include "UpdateCameraBuffer.hpp"
#include "GenerateSurfaceTexture.hpp"
#include "UpdateBufferUniforms.hpp"

// Draw
#include "Render.hpp"

// Shutdown
#include "ReleaseBindingGroup.hpp"
#include "ReleaseUniforms.hpp"
#include "ReleaseBuffers.hpp"
#include "TerminateDepthBuffer.hpp"
#include "ReleasePipeline.hpp"
#include "ReleaseDevice.hpp"
#include "ReleaseSurface.hpp"
#include "ReleaseQueue.hpp"

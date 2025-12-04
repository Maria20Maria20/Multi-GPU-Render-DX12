#pragma once
#include "d3dUtil.h"
#include "GCrossAdapterResource.h"
#include "GraphicPSO.h"
#include "GDescriptor.h"
#include "GRenderTarger.h"
#include "GTexture.h"
#include "MathHelper.h"
#include "RenderModeFactory.h"
#include "ShaderBuffersData.h"
#include "SharedSSAO.h"
#include <Rendering/Shaders/XeGTAO.h>


using namespace DirectX::SimpleMath;

using namespace PEPEngine;
using namespace Graphics;
using namespace Allocator;
using namespace Utils;

using GTAOConstants = XeGTAO::GTAOConstants;
using GTAOSettings = XeGTAO::GTAOSettings;

class XeGTAOResources final : public virtual SSAOResources
{
    GDescriptor ambientMapUAV;
    ComputePSO pso;

    void RebuildDescriptors() const override;
    void InitializeRS() override;
    void BuildPSO(const D3D12_INPUT_LAYOUT_DESC& layout) override;
public:
    const GDescriptor* GetAmbientMapUAV() const { return &ambientMapUAV; }
    const ComputePSO& GetPso() const { return pso; }

    void Initialize(const std::shared_ptr<GDevice>& Device, const D3D12_INPUT_LAYOUT_DESC& layout) override;
};

class SharedXeGTAO
{
    XeGTAOResources primeResources;
    XeGTAOResources secondResources;
    SSAOCrossResources crossResources;

    GTAOSettings gtaoSettings;
    UINT RenderTargetWidth;
    UINT RenderTargetHeight;

public:
    const XeGTAOResources& GetPrimeResources() const { return primeResources; }
    const XeGTAOResources& GetSecondResources() const { return secondResources; }
    const SSAOCrossResources& GetCrossResources() const { return crossResources; }
    const GTAOSettings& GetSettings() const { return gtaoSettings; }

    void Initialize(const std::shared_ptr<GDevice>& PrimeDevice, const std::shared_ptr<GDevice>& SecondDevice,
                    const D3D12_INPUT_LAYOUT_DESC& layout, UINT width, UINT height);
    void OnResize(UINT width, UINT height);
    void Compute(const std::shared_ptr<GCommandList>& cmdList, const std::shared_ptr<ConstantUploadBuffer<GTAOConstants>>& Constants, const XeGTAOResources& Resources) const;
};

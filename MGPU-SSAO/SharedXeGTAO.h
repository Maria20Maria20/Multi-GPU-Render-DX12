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
#include "Shaders/XeGTAO.h"


using namespace DirectX::SimpleMath;

using namespace PEPEngine;
using namespace Graphics;
using namespace Allocator;
using namespace Utils;

using GTAOConstants = XeGTAO::GTAOConstants;
using GTAOSettings = XeGTAO::GTAOSettings;

class XeGTAOResources final : public SSAOResources
{
public:
    struct Pass
    {
        std::shared_ptr<GRootSignature> RootSignature;
        std::shared_ptr<ComputePSO>     PSO;
    };
    std::shared_ptr<GDevice> device;
    GDescriptor ambientMapUAV;
    ComputePSO pso;

    void RebuildDescriptors() const;
    std::shared_ptr<GRootSignature> CreateXeGTAORootSignature(int srvCount,
    int uavCount,
    int extraCbvCount);
    void BuildPSO();
    void BuildPass(Pass& pass,
    const std::wstring& fileName,
    const std::string& entryPoint,
    int srvCount,
    int uavCount,
    int cbvCount);
    void ApplyPass(GCommandList& cmdList, Pass& pass) const;

    Pass Prefilter;
    Pass Main;
    Pass Denoise;
    Pass Composite;
    
    const GDescriptor* GetAmbientMapUAV() const { return &ambientMapUAV; }
    const ComputePSO& GetPso() const { return pso; }

    void Initialize(const std::shared_ptr<GDevice>& Device, const D3D12_INPUT_LAYOUT_DESC& layout);
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
    GTAOSettings& GetSettings() { return gtaoSettings; }

    void Initialize(const std::shared_ptr<GDevice>& PrimeDevice, const std::shared_ptr<GDevice>& SecondDevice,
                    const D3D12_INPUT_LAYOUT_DESC& layout, UINT width, UINT height);
    void OnResize(UINT width, UINT height);
    void Compute(const std::shared_ptr<GCommandList>& cmdList, const std::shared_ptr<ConstantUploadBuffer<GTAOConstants>>& Constants, const XeGTAOResources& Resources) const;
    void ExecutePass(
    const std::shared_ptr<GCommandList>& cmdList,
    const XeGTAOResources& Resources,
    const XeGTAOResources::Pass& pass,
    const std::shared_ptr<ConstantUploadBuffer<GTAOConstants>>& Constants) const;
};

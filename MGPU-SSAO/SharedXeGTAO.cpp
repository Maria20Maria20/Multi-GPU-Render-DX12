#include "SharedXeGTAO.h"


void XeGTAOResources::RebuildDescriptors() const
{
    SSAOResources::RebuildDescriptors();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = AmbientMapFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.PlaneSlice = 0;
    uavDesc.Texture2D.MipSlice = 0;
    GetAmbientMap().CreateUnorderedAccessView(&uavDesc, &ambientMapUAV);    
}

std::shared_ptr<GRootSignature> XeGTAOResources::CreateXeGTAORootSignature(int srvCount,
    int uavCount,
    int extraCbvCount)
{
    auto rs = std::make_shared<GRootSignature>();

    // b0 - всегда GTAOConstants
    rs->AddConstantBufferParameter(0);

    // SRV table
    if (srvCount > 0)
    {
        CD3DX12_DESCRIPTOR_RANGE srvTable;
        srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srvCount, 0, 0);
        rs->AddDescriptorParameter(&srvTable, 1);
    }

    // UAV table
    if (uavCount > 0)
    {
        CD3DX12_DESCRIPTOR_RANGE uavTable;
        uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uavCount, 0, 0);
        rs->AddDescriptorParameter(&uavTable, 1);
    }

    // Дополнительные CBV (b1, b2, ...)
    for (int i = 0; i < extraCbvCount; ++i)
    {
        rs->AddConstantBufferParameter(i + 1);
    }

    // Samplers
    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        0, D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        1, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    rs->AddStaticSampler(pointClamp);
    rs->AddStaticSampler(linearClamp);
    rs->Initialize(device);

    return rs;
}

void XeGTAOResources::Initialize(const std::shared_ptr<GDevice>& Device, const D3D12_INPUT_LAYOUT_DESC& layout)
{
    this->device = Device;
    SSAOResources::Initialize(Device, layout);

    ambientMapUAV = ambientMapSRV.Offset(1);

    BuildPSO();    
}

void XeGTAOResources::BuildPSO()
{
    BuildPass(Prefilter, L"Shaders\\XeGTAO_PrefilterDepths16x16.hlsl", "CSPrefilterDepths16x16",
        1,5,0);
    BuildPass(Main,      L"Shaders\\XeGTAO_MainPass.hlsl",            "CSGTAOHigh",
        2,2,0);
    BuildPass(Denoise,   L"Shaders\\XeGTAO_Denoise.hlsl",             "CSDenoisePass",
        2,1,0);
    BuildPass(Composite, L"Shaders\\XeGTAO_Composite.hlsl",           "Composite",
        1,1,1);
}

void XeGTAOResources::BuildPass(Pass& pass,
    const std::wstring& fileName,
    const std::string& entryPoint,
    int srvCount,
    int uavCount,
    int cbvCount)
{
    auto shader = std::make_unique<GShader>(
            fileName,
            ComputeShader, nullptr,
            entryPoint,
            "cs_5_1");

    shader->LoadAndCompile();

    pass.RootSignature = CreateXeGTAORootSignature(srvCount, uavCount, cbvCount);
    
    pass.PSO           = std::make_shared<ComputePSO>();
    pass.PSO->SetShader(shader.get());
    pass.PSO->SetRootSignature(*pass.RootSignature);
    pass.PSO->Initialize(device);
}

void XeGTAOResources::ApplyPass(GCommandList& cmdList, Pass& pass) const
{
    cmdList.SetComputeRootSignature(*pass.RootSignature);
    cmdList.SetPipelineState(*pass.PSO);
}


void SharedXeGTAO::Initialize(const std::shared_ptr<GDevice>& PrimeDevice, const std::shared_ptr<GDevice>& SecondDevice, const D3D12_INPUT_LAYOUT_DESC& layout, UINT width, UINT height)
{
    primeResources.Initialize(PrimeDevice, layout);
    primeResources.OnResize(width, height);

    secondResources.Initialize(SecondDevice, layout);
    secondResources.OnResize(width, height);

    crossResources.Initialize(primeResources, PrimeDevice, SecondDevice);
    crossResources.OnResize(width, height);

    OnResize(width, height);
}

void SharedXeGTAO::OnResize(UINT newWidth, UINT newHeight)
{
    if (RenderTargetWidth == newWidth && RenderTargetHeight == newHeight)
        return;

    RenderTargetWidth = newWidth;
    RenderTargetHeight = newHeight;

    primeResources.OnResize(newWidth, newHeight);
    secondResources.OnResize(newWidth, newHeight);
    crossResources.OnResize(newWidth, newHeight);
}

static UINT IntDivRoundUp	( UINT a, UINT b ) { return ( a + b - 1 ) / b; }

void SharedXeGTAO::Compute(const std::shared_ptr<GCommandList>& cmdList, const std::shared_ptr<ConstantUploadBuffer<GTAOConstants>>& Constants, const XeGTAOResources& Resources) const
{
    cmdList->StartMark(L"XeGTAO");

    cmdList->TransitionBarrier(Resources.GetAmbientMap(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->TransitionBarrier(Resources.GetDepthMap(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->FlushResourceBarriers();

    //float clearValue[] = {1.0f, 1.0f, 1.0f, 1.0f};
    //cmdList->ClearRenderTarget(Resources.GetAmbientMapRTV(), 0, clearValue);

    //cmdList->TransitionBarrier(Resources.GetDepthMap(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //cmdList->TransitionBarrier(Resources.GetRandomVectorMap(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //cmdList->TransitionBarrier(Resources.GetAmbientMap(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    //cmdList->FlushResourceBarriers();

    ExecutePass(cmdList, Resources, Resources.Prefilter, Constants);
    ExecutePass(cmdList, Resources, Resources.Main, Constants);
    ExecutePass(cmdList, Resources, Resources.Denoise, Constants);
    ExecutePass(cmdList, Resources, Resources.Composite, Constants);

    cmdList->TransitionBarrier(Resources.GetAmbientMap(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->FlushResourceBarriers();
    cmdList->EndMark();
}

void SharedXeGTAO::ExecutePass(
    const std::shared_ptr<GCommandList>& cmdList,
    const XeGTAOResources& Resources,
    const XeGTAOResources::Pass& pass,
    const std::shared_ptr<ConstantUploadBuffer<GTAOConstants>>& Constants) const
{
    Resources.ApplyPass(*cmdList, const_cast<XeGTAOResources::Pass&>(pass));
    cmdList->SetDescriptorsHeap(Resources.GetAmbientMapSRV());
    cmdList->SetComputeRootConstantBufferView(0, *Constants.get());
    cmdList->SetComputeRootDescriptorTable(1, Resources.GetDepthMapSRV());
    cmdList->SetComputeRootDescriptorTable(2, Resources.GetAmbientMapUAV());

    auto tgx = IntDivRoundUp(RenderTargetWidth, XE_GTAO_NUMTHREADS_X);
    auto tgy = IntDivRoundUp(RenderTargetHeight, XE_GTAO_NUMTHREADS_Y);
    cmdList->Dispatch(tgx, tgy, 1);
}


#pragma once
#include <d3dcompiler.h>
#include <string>

class CustomInclude : public ID3DInclude
{
public:
    explicit CustomInclude(std::wstring baseDir);
    HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;
    HRESULT Close(LPCVOID pData) override;
private:
    std::wstring m_baseDir;
};

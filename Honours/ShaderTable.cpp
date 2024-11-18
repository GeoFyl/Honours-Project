#include "stdafx.h"
#include "ShaderTable.h"

void ShaderRecord::CopyTo(void* dest) const
{
	const auto destByte = static_cast<uint8_t*>(dest);

	if (!m_ShaderIdentifier)
	{
		OutputDebugString(L"Attempting to copy a null shader record\n");
		return;
	}

	memcpy(destByte, m_ShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	if (m_LocalRootArguments.ptr)
	{
		memcpy(destByte + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, m_LocalRootArguments.ptr, m_LocalRootArguments.size);
	}
}

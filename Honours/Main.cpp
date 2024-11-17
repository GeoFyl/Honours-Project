//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "HonoursApplication.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    try {
        HonoursApplication sample(1280, 720, L"Application");
        return Win32Application::Run(&sample, hInstance, nCmdShow);
    }
    catch (DxException& exception) {
        OutputDebugString(exception.ToString().c_str());
        MessageBox(nullptr, exception.ToString().c_str(), L"HRESULT Failed", MB_OK);
        return 0;
    }
    catch (std::runtime_error error) {
        std::wstring ws(error.what(), error.what() + strlen(error.what()));
        OutputDebugString(ws.c_str());
        MessageBox(nullptr, ws.c_str(), L"RUNTIME ERROR", MB_OK);
        return 0;
    }
}

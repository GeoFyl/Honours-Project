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
#include <fstream>
#include "atlstr.h"

TestVariables test_vars_;
TestVariablesCPUOnly cpu_test_vars_;

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    try {

        int num_args;
        LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &num_args);
        
        // Command line arguments: (Test name), (particle no.), (texture res), (screen res x), (screen res y), (view distance), (scene), (implementation)

        if (num_args > 1) {
            cpu_test_vars_.test_mode_ = true;
            cpu_test_vars_.test_name_ = CW2A(args[1]);
            NUM_PARTICLES = _wtoi(args[2]);
            TEXTURE_RESOLUTION = _wtoi(args[3]);
            cpu_test_vars_.screen_res_[0] = _wtoi(args[4]);
            cpu_test_vars_.screen_res_[1] = _wtoi(args[5]);
            cpu_test_vars_.view_dist_ = std::atof(CW2A(args[6]));
            SCENE = (SceneType)_wtoi(args[7]);
            cpu_test_vars_.implementation_ = (ImplementationType)_wtoi(args[8]);
        }
        else {
            cpu_test_vars_.test_mode_ = false;
            cpu_test_vars_.test_name_ = "Default";
            cpu_test_vars_.screen_res_[0] = 1920;
            cpu_test_vars_.screen_res_[1] = 1080;
            cpu_test_vars_.view_dist_ = 1.5;
            cpu_test_vars_.implementation_ = Complex;
            SCENE = SceneWave;
            NUM_PARTICLES = 343;
            TEXTURE_RESOLUTION = 256;
        }

        HonoursApplication sample(cpu_test_vars_.screen_res_[0], cpu_test_vars_.screen_res_[1], L"Honours");
        return Win32Application::Run(&sample, hInstance, nCmdShow);
    }
    catch (DxException& exception) {
        OutputDebugString(exception.ToString().c_str());
        MessageBox(nullptr, exception.ToString().c_str(), L"HRESULT Failed", MB_OK);
        return 0;
    }
    catch (std::exception& error)
    {
        std::wstring ws(error.what(), error.what() + strlen(error.what()));
        OutputDebugString(ws.c_str());
        MessageBox(nullptr, ws.c_str(), L"EXCEPTION", MB_OK);
        return 0;
    }
}

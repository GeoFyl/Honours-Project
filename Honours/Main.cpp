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

TestVariables test_vars_;
TestVariablesCPUOnly cpu_test_vars_;

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    try {

        int num_args;
        LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &num_args);

       /* std::wofstream teststream("TestFile.txt");
        for (int i = 0; i < num_args; i++) {
            teststream << _wtoi(args[i]) << std::endl;
        }
        teststream.close();*/
        
        // Command line arguments: (screen res x), (screen res y), (view distance), (scene), (particle no.), (texture res), (cells no.), (blocks no.), (bricks per cell) 

        if (num_args > 1) {
            cpu_test_vars_.screen_res_[0] = _wtoi(args[1]);
            cpu_test_vars_.screen_res_[1] = _wtoi(args[2]);
            cpu_test_vars_.view_dist_ = _wtoi(args[3]);
            SCENE = (SceneType)_wtoi(args[4]);
            NUM_PARTICLES = _wtoi(args[5]);
            TEXTURE_RESOLUTION = _wtoi(args[6]);
            NUM_CELLS = _wtoi(args[7]);
            NUM_BLOCKS = _wtoi(args[8]);
            BRICKS_PER_CELL = _wtoi(args[9]);
        }
        else {
            cpu_test_vars_.screen_res_[0] = 1280;
            cpu_test_vars_.screen_res_[1] = 720;
            cpu_test_vars_.view_dist_ = 2;
            SCENE = SceneRandom;
            NUM_PARTICLES = 125;
            TEXTURE_RESOLUTION = 256;
            NUM_CELLS = 4096;
            NUM_BLOCKS = 64;
            BRICKS_PER_CELL = 8;

        }

        HonoursApplication sample(1280, 720, L"Application");
        return Win32Application::Run(&sample, hInstance, nCmdShow);
    }
    catch (DxException& exception) {
        OutputDebugString(exception.ToString().c_str());
        MessageBox(nullptr, exception.ToString().c_str(), L"HRESULT Failed", MB_OK);
        return 0;
    }
    /*catch (std::runtime_error error) {
        std::wstring ws(error.what(), error.what() + strlen(error.what()));
        OutputDebugString(ws.c_str());
        MessageBox(nullptr, ws.c_str(), L"RUNTIME ERROR", MB_OK);
        return 0;
    }*/
    catch (std::exception& error)
    {
        std::wstring ws(error.what(), error.what() + strlen(error.what()));
        OutputDebugString(ws.c_str());
        MessageBox(nullptr, ws.c_str(), L"EXCEPTION", MB_OK);
        return 0;

        //pSample->OnDestroy();
       // return EXIT_FAILURE;
    }
}

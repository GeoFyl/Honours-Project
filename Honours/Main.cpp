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

TestVariables test_values_;

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

        if (num_args > 1) {
            test_values_.num_particles_ = _wtoi(args[1]);
            test_values_.texture_res_ = _wtoi(args[2]);
            test_values_.num_cells_ = _wtoi(args[3]);
            test_values_.num_blocks_ = _wtoi(args[4]);
            test_values_.bricks_per_cell_ = _wtoi(args[5]);
        }
        else {
            test_values_.num_particles_ = 200;
            test_values_.texture_res_ = 256;
            test_values_.num_cells_ = 4096;
            test_values_.num_blocks_ = 64;
            test_values_.bricks_per_cell_ = 8;
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

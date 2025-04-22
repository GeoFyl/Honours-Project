:: cd directory

:: Default values:
:: - Particle num: 343
:: - Texture res: 256
:: - Screen res: 1920 x 1080
:: - View distance: 1.5
:: - Scene: Wave (2)
:: - All implementations tested (0, 1, 2)

:: 48 tests

:: ----- Default ------
Honours.exe Default_Naive 343 256 1920 1080 1.5 2 0
Honours.exe Default_Simple 343 256 1920 1080 1.5 2 1
Honours.exe Default_Complex 343 256 1920 1080 1.5 2 2


:: ------ Variable tested: scene -------
Honours.exe Scene_Rand_Naive 343 256 1920 1080 1.5 0 0
Honours.exe Scene_Rand_Simple 343 256 1920 1080 1.5 0 1
Honours.exe Scene_Rand_Complex 343 256 1920 1080 1.5 0 2

Honours.exe Scene_Grid_Naive 343 256 1920 1080 1.5 1 0
Honours.exe Scene_Grid_Simple 343 256 1920 1080 1.5 1 1
Honours.exe Scene_Grid_Complex 343 256 1920 1080 1.5 1 2

:: Default == Scene_Wave


:: ------ Variable tested: particle num -------
Honours.exe Particles_1_Naive 1 256 1920 1080 1.5 2 0
Honours.exe Particles_1_Simple 1 256 1920 1080 1.5 2 1
Honours.exe Particles_1_Complex 1 256 1920 1080 1.5 2 2

Honours.exe Particles_27_Naive 27 256 1920 1080 1.5 2 0
Honours.exe Particles_27_Simple 27 256 1920 1080 1.5 2 1
Honours.exe Particles_27_Complex 27 256 1920 1080 1.5 2 2

:: Default == Particles_343

Honours.exe Particles_1000_Naive 1000 256 1920 1080 1.5 2 0
Honours.exe Particles_1000_Simple 1000 256 1920 1080 1.5 2 1
Honours.exe Particles_1000_Complex 1000 256 1920 1080 1.5 2 2

Honours.exe Particles_10648_Naive 10648 256 1920 1080 1.5 2 0
Honours.exe Particles_10648_Simple 10648 256 1920 1080 1.5 2 1
Honours.exe Particles_10648_Complex 10648 256 1920 1080 1.5 2 2


:: ------ Variable tested: texture res -------
:: Naive doesn't use a texture
Honours.exe Tex_128_Simple 343 128 1920 1080 1.5 2 1
Honours.exe Tex_128_Complex 343 128 1920 1080 1.5 2 2

:: Default == Tex_256

Honours.exe Tex_512_Simple 343 512 1920 1080 1.5 2 1
Honours.exe Tex_512_Complex 343 512 1920 1080 1.5 2 2

Honours.exe Tex_768_Simple 343 768 1920 1080 1.5 2 1
Honours.exe Tex_768_Complex 343 768 1920 1080 1.5 2 2


:: ------ Variable tested: screen res -------
Honours.exe Screen_720p_Naive 343 256 1280 720 1.5 2 0
Honours.exe Screen_720p_Simple 343 256 1280 720 1.5 2 1
Honours.exe Screen_720p_Complex 343 256 1280 720 1.5 2 2

:: Default == Screen_1080p

Honours.exe Screen_1440p_Naive 343 256 2560 1440 1.5 2 0
Honours.exe Screen_1440p_Simple 343 256 2560 1440 1.5 2 1
Honours.exe Screen_1440p_Complex 343 256 2560 1440 1.5 2 2

Honours.exe Screen_4k_Naive 343 256 3840 2160 1.5 2 0
Honours.exe Screen_4k_Simple 343 256 3840 2160 1.5 2 1
Honours.exe Screen_4k_Complex 343 256 3840 2160 1.5 2 2


:: ------ Variable tested: view distance -------
Honours.exe ViewDist_1_Naive 343 256 1920 1080 1 2 0
Honours.exe ViewDist_1_Simple 343 256 1920 1080 1 2 1
Honours.exe ViewDist_1_Complex 343 256 1920 1080 1 2 2

:: Default == ViewDist_1-5

Honours.exe ViewDist_2_Naive 343 256 1920 1080 2 2 0
Honours.exe ViewDist_2_Simple 343 256 1920 1080 2 2 1
Honours.exe ViewDist_2_Complex 343 256 1920 1080 2 2 2

Honours.exe ViewDist_2-5_Naive 343 256 1920 1080 2.5 2 0
Honours.exe ViewDist_2-5_Simple 343 256 1920 1080 2.5 2 1
Honours.exe ViewDist_2-5_Complex 343 256 1920 1080 2.5 2 2



@echo msgbox "Testing Complete" > %tmp%\tmp.vbs
@cscript /nologo %tmp%\tmp.vbs
del %tmp%\tmp.vbs
@REM Notes to copy paste

"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd C:\prog\vulkan\Vulkan-Samples\


@REM cmake config from samples documentation
rmdir /s /q build\windows_default
sccache --zero-stats
cmake -G "Visual Studio 17 2022" -A x64 -S . -Bbuild/windows_default
@echo Started: %date% %time%
cmake --build build/windows_default --config Debug --target vulkan_samples
@echo End: %date% %time%
sccache --show-stats


@REM cmake config using ninja
rmdir /s /q build\windows_ninja
sccache --zero-stats
cmake -G Ninja . -Bbuild/windows_ninja
@echo Started: %date% %time%
cmake --build build/windows_ninja --config Debug --target vulkan_samples
@echo End: %date% %time%
sccache --show-stats

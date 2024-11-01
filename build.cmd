@echo off

cmake --preset vs2022-windows --check-stamp-file "build/CMakeFiles/generate.stamp" || goto :error
cmake --build build --config Release || goto :error
cmake --install build --component "fomod" --prefix "build-fomod" || goto :error
cmake --install build --component "SKSEPlugin" --prefix "build-fomod/Core/SkyrimSE" || goto :error

cmake --preset vs2022-windows-vr --check-stamp-file "buildVR/CMakeFiles/generate.stamp" || goto :error
cmake --build buildVR --target "StaffEnchanting" --config Release || goto :error
cmake --install buildVR --component "SKSEPlugin" --prefix "build-fomod/Core/SkyrimVR" || goto :error

pushd build-fomod
7z a -r -t7Z "..\StaffEnchanting.7z" *
popd

goto :EOF

:error
exit /b %errorlevel%

@ECHO off

SETLOCAL enabledelayedexpansion
SET BATCH_NAME=%~nx0
SET PROJ_PATH=%~dp0

rem Default flag
SET BUILD_TYPE=Debug
SET FLAG=-D CMAKE_BUILD_TYPE=%BUILD_TYPE% -D CMAKE_INSTALL_PREFIX=%PROJ_PATH%\install

call "compile_shader.bat"


@REM check for the visual studio path
for %%e in (Professional Enterprise) do (
    IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\%%e\MSBuild\Current\Bin\MSBuild.exe" (
        set "MSVC_ENV_SETUP=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\%%e\VC\Auxiliary\Build\vcvars64.bat"
    ) 
)

IF NOT EXIST "%MSVC_ENV_SETUP%" GOTO MSBuild_error
call "%MSVC_ENV_SETUP%"

@REM force re-creation of build folder
IF EXIST %PROJ_PATH%\build RMDIR /Q /S %PROJ_PATH%\build
mkdir %PROJ_PATH%\build

cd %PROJ_PATH%\build
REM conan install .. -s build_type=%BUILD_TYPE% -r virtuos
REM IF NOT %ERRORLEVEL%==0  GOTO conan_error


cmake -G "Visual Studio 16 2019"  %FLAG% ..
IF NOT %ERRORLEVEL%==0  GOTO cmake_error
echo START: %time%
set BUILD_PATH=.
REM cmake --build %BUILD_PATH% --target install --config %BUILD_TYPE% --clean-first --parallel -j12
echo END: %time%
GOTO :eof

:cmake_error
ECHO.
ECHO CMake Failed!!!
ECHO.
EXIT /B 1
GOTO :eof

:MSBuild_error
ECHO.
ECHO The MSVC was not found at the expected location:
ECHO.
ECHO Please install the MSVC Professional/Enterprise
ECHO.
EXIT /B 3
GOTO :eof

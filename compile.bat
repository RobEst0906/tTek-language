@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: compile.bat source.ttek
    exit /b 1
)

set "SOURCE=%~1"
set "BASENAME=%~n1"
set "SOURCE_DIR=%~dp1"
set "ROOT_DIR=%~dp0"
set "BUILD_DIR=%ROOT_DIR%build"
set "COMPILER=%BUILD_DIR%\ttek_compiler.exe"

set "EXE_PATH=%SOURCE_DIR%%BASENAME%.exe"
set "OBJ_PATH=%SOURCE_DIR%%BASENAME%.o"

echo Building compiler...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"
mingw32-make
cd /d "%ROOT_DIR%"
if not exist "%COMPILER%" (
    echo Failed to build compiler.
    exit /b 1
)
"%COMPILER%" "%SOURCE%"
echo Compiler exit code: %errorlevel%
echo Checking for .o file:
dir "%SOURCE_DIR%*.o"
echo Checking for .exe file:
dir "%SOURCE_DIR%*.exe"

echo Compiling %SOURCE%...
"%COMPILER%" "%SOURCE%"
if errorlevel 1 (
    echo Compilation failed.
    exit /b 1
)

echo Running %BASENAME%.exe...
echo.
if exist "%EXE_PATH%" (
    "%EXE_PATH%"
) else (
    echo Executable not found at %EXE_PATH%. Trying to link manually...
    if exist "%OBJ_PATH%" (
        gcc "%OBJ_PATH%" -o "%EXE_PATH%" -mconsole
        if exist "%EXE_PATH%" (
            "%EXE_PATH%"
        ) else (
            echo Linking failed.
            exit /b 1
        )
    ) else (
        echo Object file not found at %OBJ_PATH%. Something went wrong.
        exit /b 1
    )
)

exit /b 0
@echo off
REM
REM Copyright 2005-2016 Intel Corporation.  All Rights Reserved.
REM
REM This file is part of Threading Building Blocks. Threading Building Blocks is free software;
REM you can redistribute it and/or modify it under the terms of the GNU General Public License
REM version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
REM distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
REM implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
REM See  the GNU General Public License for more details.   You should have received a copy of
REM the  GNU General Public License along with Threading Building Blocks; if not, write to the
REM Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA
REM
REM As a special exception,  you may use this file  as part of a free software library without
REM restriction.  Specifically,  if other files instantiate templates  or use macros or inline
REM functions from this file, or you compile this file and link it with other files to produce
REM an executable,  this file does not by itself cause the resulting executable to be covered
REM by the GNU General Public License. This exception does not however invalidate any other
REM reasons why the executable file might be covered by the GNU General Public License.
REM

set SCRIPT_NAME=%~nx0
set TBB_BIN_DIR=%~d0%~p0
set TBBROOT=%TBB_BIN_DIR%..

:: Set the default arguments
set TBB_TARGET_ARCH=
set TBB_TARGET_VS=

:ParseArgs
:: Parse the incoming arguments
if /i "%1"==""        goto SetEnv
if /i "%1"=="ia32"         (set TBB_TARGET_ARCH=ia32)    & shift & goto ParseArgs
if /i "%1"=="intel64"      (set TBB_TARGET_ARCH=intel64) & shift & goto ParseArgs
if /i "%1"=="vs2010"       (set TBB_TARGET_VS=vc10)      & shift & goto ParseArgs
if /i "%1"=="vs2012"       (set TBB_TARGET_VS=vc11)      & shift & goto ParseArgs
if /i "%1"=="vs2013"       (set TBB_TARGET_VS=vc12)      & shift & goto ParseArgs
if /i "%1"=="vs2015"       (set TBB_TARGET_VS=vc14)      & shift & goto ParseArgs
if /i "%1"=="all"          (set TBB_TARGET_VS=vc_mt)     & shift & goto ParseArgs
:: for any other incoming arguments values
goto Syntax

:SetEnv
:: target architecture is a required argument
if "%TBB_TARGET_ARCH%"=="" goto Syntax
:: TBB_TARGET_VS default value is 'vc_mt'
if "%TBB_TARGET_VS%"=="" set TBB_TARGET_VS=vc_mt

set TBB_ARCH_PLATFORM=%TBB_TARGET_ARCH%\%TBB_TARGET_VS%
if exist "%TBB_BIN_DIR%\%TBB_ARCH_PLATFORM%\tbb.dll" set PATH=%TBB_BIN_DIR%\%TBB_ARCH_PLATFORM%;%PATH%
if exist "%TBBROOT%\..\redist\%TBB_TARGET_ARCH%\tbb\%TBB_TARGET_VS%\tbb.dll" set PATH=%TBBROOT%\..\redist\%TBB_TARGET_ARCH%\tbb\%TBB_TARGET_VS%;%PATH%
set LIB=%TBBROOT%\lib\%TBB_ARCH_PLATFORM%;%LIB%
set INCLUDE=%TBBROOT%\include;%INCLUDE%
set CPATH=%TBBROOT%\include;%CPATH%
if exist "%TBBROOT%\lib\mic\libtbb.so" (
    set "MIC_LIBRARY_PATH=%TBBROOT%\lib\mic;%MIC_LIBRARY_PATH%"
    set "MIC_LD_LIBRARY_PATH=%TBBROOT%\lib\mic;%MIC_LD_LIBRARY_PATH%"
)
if not "%ICPP_COMPILER13%"=="" set TBB_CXX=icl.exe
if not "%ICPP_COMPILER14%"=="" set TBB_CXX=icl.exe
if not "%ICPP_COMPILER15%"=="" set TBB_CXX=icl.exe
if not "%ICPP_COMPILER16%"=="" set TBB_CXX=icl.exe
goto End

:Syntax
echo Syntax:
echo  %SCRIPT_NAME% ^<arch^> ^<vs^>
echo    ^<arch^> must be one of the following
echo        ia32         : Set up for IA-32  architecture
echo        intel64      : Set up for Intel(R) 64  architecture
echo    ^<vs^> should be one of the following
echo        vs2010      : Set to use with Microsoft Visual Studio 2010 runtime DLLs
echo        vs2012      : Set to use with Microsoft Visual Studio 2012 runtime DLLs
echo        vs2013      : Set to use with Microsoft Visual Studio 2013 runtime DLLs
echo        vs2015      : Set to use with Microsoft Visual Studio 2015 runtime DLLs
echo        all         : Set to use TBB statically linked with Microsoft Visual C++ runtime
echo    if ^<vs^> is not set TBB statically linked with Microsoft Visual C++ runtime will be used.
exit /B 1

:End
exit /B 0
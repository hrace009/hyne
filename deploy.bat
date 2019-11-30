@echo off

set OUTPUT_DIR=deploy
set EXE_PATH=release\hyne.exe
set LIB_DIR=%QTDIR%\bin
set QT_TR_DIR=%QTDIR%\translations
set LANGUAGES=en ja

rem Create target directory
if not exist %OUTPUT_DIR% mkdir %OUTPUT_DIR%

rem Deploy DLLs
%LIB_DIR%\windeployqt.exe --force --release --dir %OUTPUT_DIR% --no-quick-import --no-translations --no-system-d3d-compiler --no-opengl --no-3dcore --no-3drenderer --no-webkit2 --no-angle --no-svg --no-webkit %EXE_PATH%

rem Removing unused DLLs
del /q %OUTPUT_DIR%\opengl32sw.dll

rem Deploy Translations
for %%l in (%LANGUAGES%) do (
    %LIB_DIR%\lconvert -o %OUTPUT_DIR%\qt_%%l.qm -no-obsolete -no-ui-lines %QT_TR_DIR%\qtbase_%%l.qm
    echo "Create %QT_TR_DIR%\qtbase_%%l.qm"
)

rem Deploy Exe
xcopy /y %EXE_PATH% %OUTPUT_DIR%

rem Compress Exe and DLLs. Note: DLLs in platforms/ directory should not be compressed.
rem upx %OUTPUT_DIR%\*.dll %OUTPUT_DIR%\Hyne.exe

@echo off
setlocal

rem Keep the preview self-contained and separate from an installed QElectroTech.
set "preview_dir=%~dp0"
cd /d "%preview_dir%"
if not exist "conf" (
  mkdir "conf" 2>nul
  if errorlevel 1 (
    echo Unable to create the isolated preview configuration folder:
    echo %preview_dir%conf
    pause
    exit /b 1
  )
)

start "" "%preview_dir%qelectrotech.exe" ^
  -platform windows:fontengine=freetype ^
  --common-elements-dir=elements/ ^
  --common-tbt-dir=titleblocks/ ^
  --lang-dir=l10n/ ^
  --config-dir=conf/ ^
  --data-dir=conf/ ^
  -style fusion %*

endlocal

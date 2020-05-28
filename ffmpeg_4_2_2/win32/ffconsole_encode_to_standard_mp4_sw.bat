@echo off

rem echo %~F0
rem echo %~F1

Set filename=%~F0
For %%A in ("%filename%") do (
    Set Folder=%%~dpA
    Set Name=%%~nxA
)

Set filename1=%~F1
For %%B in ("%filename1%") do (
    Set Folder1=%%~dpB
    Set Name1=%%~nxB
)

rem echo "%Folder1%"
rem echo "%Name1%"
rem pause

cd "%Folder1%"

TITLE Mp4 Video 1 Click - FFMPEG v.4.2.2 32-bit - Encode to standard h264 mp4 (software libx264)

set PATH=%Folder%;%SYSTEMROOT%\SysWOW64;%SYSTEMROOT%\System32

set FFREPORT=file=%Name1%.encode_to_standard_h264_sw.mp4.log:level=32
rem echo %FFREPORT%

ffmpeg.exe -i "%Name1%" -vf "format=pix_fmts=yuv420p" -af "dynaudnorm" -c:v libx264 -profile:v baseline -preset ultrafast -crf 30 -c:a aac -y "%Name1%.encode_to_standard_h264_sw.mp4"

IF %ERRORLEVEL% EQU 0 goto SUCCESS_LABEL

echo See "%Name1%.encode_to_standard_h264_sw.mp4.log" for details.
pause

goto END_LABEL

:SUCCESS_LABEL

del /Q /F "%Name1%.encode_to_standard_h264_sw.mp4.log"

:END_LABEL

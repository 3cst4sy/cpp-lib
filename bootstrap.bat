@ECHO OFF
setlocal

ECHO Create Folder Structure
if not exist "download\" mkdir download
if not exist "lib\" mkdir lib

ECHO Download Libs
ECHO TarTool_2_0_Beta.zip
Call :Download "http://download-codeplex.sec.s-msft.com/Download/Release?ProjectName=tartool&DownloadId=363994&FileTime=129779009509230000&Build=21031" "download\TarTool_2_0_Beta.zip" C8-F4-05-6C-1F-F3-84-68-3E-A9-06-72-5F-A0-17-4C-75-4A-A4-DE
ECHO boost_1_60_0.tar.gz
Call :Download "https://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.gz/download?use_mirror=autoselect" "download\boost_1_60_0.tar.gz" AC-74-DB-13-24-E6-50-7A-30-9C-5B-13-9A-EA-41-AF-62-4C-81-10
ECHO eigen_3.2.8.zip
Call :Download "http://bitbucket.org/eigen/eigen/get/3.2.8.zip" "download\eigen_3.2.8.zip" 14-C2-89-CF-4D-16-49-86-A8-1B-A5-A4-94-F8-FC-8E-59-A1-6B-B5
ECHO pngpp-0.2.9.tar.gz
Call :Download "http://download.savannah.gnu.org/releases/pngpp/png++-0.2.9.tar.gz" "download\pngpp-0.2.9.tar.gz" 02-4C-E6-4B-0A-47-39-84-63-7A-B2-BE-1E-F5-35-CC-74-44-88-E9
ECHO libpng-1.6.21.zip
Call :Download "https://sourceforge.net/projects/libpng/files/libpng16/1.6.21/lpng1621.zip" "download\libpng-1.6.21.zip" 76-48-D7-5C-E0-25-BB-7F-2F-EB-B7-54-7B-63-7B-52-3C-6E-F3-C0

ECHO Init BoostLib
cd lib\boost_1_60_0\
Call bootstrap.bat
b2 --toolset=msvc variant=debug,release link=static threading=multi runtime-link=static stage
cd ..\..\

ECHO Init libpng
copy lib\lpng1621\scripts\pnglibconf.h.prebuilt lib\lpng1621\pnglibconf.h

goto :end

ECHO Extract Libs
ECHO TarTool_2_0_Beta.zip
ECHO May take a while...
Call :UnZipFile "lib\" "download\TarTool_2_0_Beta.zip"
ECHO eigen_3.2.8.zip
ECHO May take a while...
Call :UnZipFile "lib\" "download\eigen_3.2.8.zip"
ECHO libpng-1.6.21.zip
ECHO May take a while...
Call :UnZipFile "lib\" "download\libpng-1.6.21.zip"
ECHO boost_1_60_0.tar.gz
ECHO May take a while...
lib\TarTool_Beta\TarTool.exe "download\boost_1_60_0.tar.gz" "lib"
ECHO pngpp-0.2.9.tar.gz
ECHO May take a while...
lib\TarTool_Beta\TarTool.exe "download\pngpp-0.2.9.tar.gz" "lib"

:SKIPEXTRACT



goto :end

:Download <URI> <toFile> <hash>
setlocal
if not "%3"=="" (
	for /f "delims=" %%a in ('powershell -Command "& {$sha1 = New-Object System.Security.Cryptography.SHA1CryptoServiceProvider; [System.BitConverter]::ToString( $sha1.ComputeHash([System.IO.File]::ReadAllBytes(\"%2\")))}"') do set VAR=%%a
) else goto :_download
if "%3" == "%VAR%" (
	echo File already exists
	goto :end
)
:_download
set COMMAND=^& {(New-Object System.Net.WebClient).DownloadFile('%uri%','%tofile%')}
powershell -Command "%COMMAND%"
goto :end

:UnZipFile <ExtractTo> <newzipfile>
set vbs="_.vbs"
if exist %vbs% del /f /q %vbs%
>%vbs%  echo Set fso = CreateObject("Scripting.FileSystemObject")
>>%vbs% echo If NOT fso.FolderExists(%1) Then
>>%vbs% echo fso.CreateFolder(%1)
>>%vbs% echo End If
>>%vbs% echo set objShell = CreateObject("Shell.Application")
>>%vbs% echo set FilesInZip=objShell.NameSpace(%2).items
>>%vbs% echo objShell.NameSpace(%1).CopyHere(FilesInZip)
>>%vbs% echo Set fso = Nothing
>>%vbs% echo Set objShell = Nothing
cscript //nologo %vbs%
if exist %vbs% del /f /q %vbs%
goto :end

:end
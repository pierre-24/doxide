﻿$ErrorActionPreference = 'Stop'
$toolsDir   = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"

$packageArgs = @{
  packageName   = $env:ChocolateyPackageName
  unzipLocation = $toolsDir
  fileType      = 'EXE'
  file          = ''
  file64        = "$toolsDir\doxide-installer.exe"
  url           = ''
  url64bit      = 'https://download.indii.org/win/doxide-installer.exe'
  softwareName  = 'doxide.install'
  checksum      = ''
  checksumType  = ''
  checksum64    = 'E578BC535C7E330BEFC646FFCBDA7F3536A6ABC223955F9ECF6319BB85346CCD'
  checksumType64= 'sha256'
  silentArgs   = '/S'
}

Install-ChocolateyInstallPackage @packageArgs

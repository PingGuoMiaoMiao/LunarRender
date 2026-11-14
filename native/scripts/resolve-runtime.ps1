[CmdletBinding()]
param(
  [string]$RepoRoot = '',
  [string]$Example = 'textured_cube'
)

$ErrorActionPreference = 'Stop'

# PowerShell 5.1 does not populate $PSScriptRoot inside parameter defaults, so
# the repository root is resolved after the param block.
if ([string]::IsNullOrEmpty($RepoRoot)) {
  $RepoRoot = Split-Path -Parent $PSScriptRoot
}
if ([string]::IsNullOrEmpty($RepoRoot)) {
  throw 'Cannot locate the LunarRenderNative module root; pass -RepoRoot explicitly.'
}

$resolvedRoot = [IO.Path]::GetFullPath($RepoRoot)
$buildRoot = Join-Path $resolvedRoot '_build\native\release\build'
if (!(Test-Path -LiteralPath $buildRoot -PathType Container)) {
  throw "LunarRender release build directory was not found: $buildRoot. Run .\scripts\build.ps1 first."
}

$executableName = "$Example.exe"
$executables = @(Get-ChildItem -LiteralPath $buildRoot -Recurse -Filter $executableName -File | Sort-Object -Property FullName)
if ($executables.Count -eq 0) {
  throw "LunarRender example '$Example' was not found below: $buildRoot. Run .\scripts\build.ps1 first."
}
if ($executables.Count -gt 1) {
  $paths = ($executables | ForEach-Object { $_.FullName }) -join [Environment]::NewLine
  throw "Multiple '$executableName' executables were found; refusing to choose implicitly:`n$paths"
}

$executables[0].FullName

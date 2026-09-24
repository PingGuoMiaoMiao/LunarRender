[CmdletBinding()]
param(
  [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'

$resolvedRoot = [IO.Path]::GetFullPath($RepoRoot)
$buildRoot = Join-Path $resolvedRoot '_build\native\release\build'
if (!(Test-Path -LiteralPath $buildRoot -PathType Container)) {
  throw "LunarRender release build directory was not found: $buildRoot. Run .\scripts\build.ps1 first."
}

$executables = @(Get-ChildItem -LiteralPath $buildRoot -Recurse -Filter 'lunarrender.exe' -File | Sort-Object -Property FullName)
if ($executables.Count -eq 0) {
  throw "LunarRender executable was not found below: $buildRoot. Run .\scripts\build.ps1 first."
}
if ($executables.Count -gt 1) {
  $paths = ($executables | ForEach-Object { $_.FullName }) -join [Environment]::NewLine
  throw "Multiple LunarRender executables were found; refusing to choose implicitly:`n$paths"
}

$executables[0].FullName

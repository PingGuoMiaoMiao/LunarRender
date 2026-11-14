[CmdletBinding()]
param(
  [string]$RepoRoot = ''
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrEmpty($RepoRoot)) {
  $RepoRoot = Split-Path -Parent $PSScriptRoot
}
if ([string]::IsNullOrEmpty($RepoRoot)) {
  throw 'Cannot locate the LunarRenderNative module root; pass -RepoRoot explicitly.'
}

$resolver = Join-Path $RepoRoot 'scripts\resolve-runtime.ps1'
foreach ($example in @('triangle', 'textured_cube', 'multi_mesh')) {
  $path = & $resolver -RepoRoot $RepoRoot -Example $example
  Write-Output ("RUN {0}" -f $path)
  & $path --self-test
  if ($LASTEXITCODE -ne 0) {
    throw "Self-test failed for $example with exit code $LASTEXITCODE."
  }
}

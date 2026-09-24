$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$thirdPartyBuild = Join-Path $repoRoot 'third_party\build'

$moonCommand = Get-Command moon -ErrorAction Stop
$cmakeCommand = Get-Command cmake -ErrorAction Stop
$vsWhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vsWhere -PathType Leaf)) {
  throw "Visual Studio locator was not found: $vsWhere"
}

$vsInstallPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace(($vsInstallPath -join ''))) {
  throw 'Visual Studio 2022 with the MSVC C/C++ workload was not found.'
}
$vsDevCmd = Join-Path ($vsInstallPath | Select-Object -First 1) 'Common7\Tools\VsDevCmd.bat'
if (!(Test-Path -LiteralPath $vsDevCmd -PathType Leaf)) {
  throw "Visual Studio developer command file was not found: $vsDevCmd"
}

$devEnvironment = & cmd.exe /d /s /c "call `"$vsDevCmd`" -arch=x64 && set"
if ($LASTEXITCODE -ne 0) {
  throw 'Failed to initialize the Visual Studio x64 developer environment.'
}
foreach ($line in $devEnvironment) {
  $text = [string]$line
  $separator = $text.IndexOf('=')
  if ($separator -gt 0) {
    $name = $text.Substring(0, $separator)
    $value = $text.Substring($separator + 1)
    [Environment]::SetEnvironmentVariable($name, $value, 'Process')
  }
}

& $cmakeCommand.Source -S $repoRoot -B $thirdPartyBuild -G 'Visual Studio 17 2022' -A x64 -DBUILD_SHARED_LIBS=OFF
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
& $cmakeCommand.Source --build $thirdPartyBuild --config Release
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

& $moonCommand.Source -C $repoRoot check --target native
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
& $moonCommand.Source -C $repoRoot test --target native
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
& $moonCommand.Source -C $repoRoot build --target native --release 'cmd/lunarrender'
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

$runtimeResolver = Join-Path $repoRoot 'scripts\resolve-runtime.ps1'
$runtimeExecutable = & $runtimeResolver -RepoRoot $repoRoot
Write-Output ("LunarRender executable: {0}" -f $runtimeExecutable)
exit 0

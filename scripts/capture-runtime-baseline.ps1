[CmdletBinding()]
param(
  [string]$ExecutablePath = '',
  [Int64]$Seed = 0
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$defaultExecutable = Join-Path $repoRoot '_build\native\release\build\cmd\moonmc\moonmc.exe'
$runtimeExecutable = if ([string]::IsNullOrWhiteSpace($ExecutablePath)) {
  $defaultExecutable
} else {
  [IO.Path]::GetFullPath($ExecutablePath)
}

if (!(Test-Path -LiteralPath $runtimeExecutable -PathType Leaf)) {
  throw "LunarRender executable was not found: $runtimeExecutable. Run .\scripts\build.ps1 first."
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$baselineRoot = Join-Path $repoRoot 'artifacts\runtime-baseline'
$runRoot = Join-Path $baselineRoot $timestamp
New-Item -ItemType Directory -Force -Path $runRoot | Out-Null

$stdoutPath = Join-Path $runRoot 'stdout.log'
$stderrPath = Join-Path $runRoot 'stderr.log'
$metadataPath = Join-Path $runRoot 'run.json'
$argumentList = @('--seed', $Seed.ToString([Globalization.CultureInfo]::InvariantCulture))

$metadata = [ordered]@{
  executable = $runtimeExecutable
  working_directory = $repoRoot
  seed = $Seed
  resource_pack = 'default unless the existing matching save selects another pack'
  arguments = $argumentList
  started_at = (Get-Date).ToString('o')
  stdout = $stdoutPath
  stderr = $stderrPath
  visual_target = (Join-Path $repoRoot 'docs\visual-target.md')
}
$metadata | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $metadataPath -Encoding UTF8

$process = Start-Process -FilePath $runtimeExecutable -ArgumentList $argumentList -WorkingDirectory $repoRoot -PassThru -WindowStyle Normal -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath

Write-Output ("LunarRender baseline started. PID={0}" -f $process.Id)
Write-Output ("Run record: {0}" -f $metadataPath)
Write-Output ("Stdout: {0}" -f $stdoutPath)
Write-Output ("Stderr: {0}" -f $stderrPath)
Write-Output '操作：F8 切换第三人称，F7 切换资源包；Esc 退出。请按 docs/visual-target.md 记录观察结果。'

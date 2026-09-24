[CmdletBinding()]
param(
  [string]$ExecutablePath = ''
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$defaultExecutable = Join-Path $repoRoot '_build\native\release\build\cmd\lunarrender\lunarrender.exe'
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
$metadata = [ordered]@{
  executable = $runtimeExecutable
  working_directory = $repoRoot
  started_at = (Get-Date).ToString('o')
  stdout = $stdoutPath
  stderr = $stderrPath
}
$metadata | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $metadataPath -Encoding UTF8

$process = Start-Process -FilePath $runtimeExecutable -WorkingDirectory $repoRoot -PassThru -WindowStyle Normal -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath

Write-Output ("LunarRender runtime started. PID={0}" -f $process.Id)
Write-Output ("Run record: {0}" -f $metadataPath)
Write-Output ("Stdout: {0}" -f $stdoutPath)
Write-Output ("Stderr: {0}" -f $stderrPath)
Write-Output '操作：按 Esc 退出示例窗口。'

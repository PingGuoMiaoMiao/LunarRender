[CmdletBinding()]
param(
  [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

# Publishing the nested native module requires the repository-root `.moonignore`
# to be out of the way: it excludes `native/` from the core package, and the
# packager would otherwise drop every native file as well. The file is restored
# in `finally`, so an aborted run leaves the checkout intact.
$repoRoot = Split-Path -Parent $PSScriptRoot
$nativeRoot = Join-Path $repoRoot 'native'
$moonIgnore = Join-Path $repoRoot '.moonignore'
$stash = Join-Path $repoRoot '.moonignore.publish-stash'

if (!(Test-Path -LiteralPath $moonIgnore -PathType Leaf)) {
  throw "Repository-root .moonignore was not found: $moonIgnore"
}
if (Test-Path -LiteralPath $stash) {
  throw "A previous publish run left '$stash' behind; restore it before publishing again."
}
if (!(Test-Path -LiteralPath $nativeRoot -PathType Container)) {
  throw "Native module directory was not found: $nativeRoot"
}

$moonCommand = Get-Command moon -ErrorAction Stop

Move-Item -LiteralPath $moonIgnore -Destination $stash
try {
  $arguments = @('-C', $nativeRoot, 'publish')
  if ($DryRun) {
    $arguments += '--dry-run'
  }
  & $moonCommand.Source @arguments
  if ($LASTEXITCODE -ne 0) {
    throw "moon publish failed with exit code $LASTEXITCODE."
  }
} finally {
  Move-Item -LiteralPath $stash -Destination $moonIgnore -Force
}

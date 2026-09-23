<#
.SYNOPSIS
    Baldies Resolution & In-Game Viewport Selector Utility (PowerShell)
.DESCRIPTION
    Configures both ddraw.ini and baldies.exe so that the game's internal camera,
    viewport, DirectDraw surface, and tile rendering dynamically scale to match the window size.
#>

param (
    [Parameter(Position=0)]
    [int]$Width = 0,

    [Parameter(Position=1)]
    [int]$Height = 0,

    [Parameter(Position=2)]
    [string]$Fullscreen = "false"
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ScriptDir) { $ScriptDir = Get-Location }

$IniPath  = Join-Path $ScriptDir "ddraw.ini"
$GameExe  = Join-Path $ScriptDir "baldies.exe"
$Win11Exe = Join-Path $ScriptDir "baldies_win11.exe"

$Presets = [ordered]@{
    "0" = @{ Name="640 x 480";   W=640;  H=480;  FS=$false; Desc="Original Classic (Default Engine Res)" }
    "1" = @{ Name="1024 x 768";  W=1024; H=768;  FS=$false; Desc="Classic 4:3 Window (Expanded View)" }
    "2" = @{ Name="1280 x 960";  W=1280; H=960;  FS=$false; Desc="Standard HD 4:3 Window (Recommended)" }
    "3" = @{ Name="1440 x 1080"; W=1440; H=1080; FS=$false; Desc="Full-Height 1080p 4:3 Window" }
    "4" = @{ Name="1600 x 1200"; W=1600; H=1200; FS=$false; Desc="UXGA 4:3 Window (2.5x World View)" }
    "5" = @{ Name="1920 x 1440"; W=1920; H=1440; FS=$false; Desc="QHD 4:3 Window (3x World View)" }
    "6" = @{ Name="1920 x 1080"; W=1920; H=1080; FS=$true;  Desc="Borderless Fullscreen (1080p Widescreen)" }
    "7" = @{ Name="2560 x 1440"; W=2560; H=1440; FS=$true;  Desc="Borderless Fullscreen (1440p Widescreen)" }
    "8" = @{ Name="3840 x 2160"; W=3840; H=2160; FS=$true;  Desc="Borderless Fullscreen (4K UHD Widescreen)" }
}

function Patch-ExecutableResolution([string]$exePath, [int]$W, [int]$H) {
    if (-not (Test-Path $exePath)) { return $false }

    try {
        $bytes = [System.IO.File]::ReadAllBytes($exePath)

        $wU16 = [uint16]$W
        $hU16 = [uint16]$H
        $hSub32 = [uint16][Math]::Max(0, $H - 32)

        $wB = [System.BitConverter]::GetBytes($wU16)
        $hB = [System.BitConverter]::GetBytes($hU16)
        $hSub32B = [System.BitConverter]::GetBytes($hSub32)

        $w32B = [System.BitConverter]::GetBytes([uint32]$W)
        $h32B = [System.BitConverter]::GetBytes([uint32]$H)

        # 1. DirectDraw & Engine resolution assignments
        [System.Array]::Copy($wB, 0, $bytes, 0x00621A, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x006223, 2)
        [System.Array]::Copy($hSub32B, 0, $bytes, 0x006503, 2)
        [System.Array]::Copy($h32B, 0, $bytes, 0x006755, 4)
        [System.Array]::Copy($w32B, 0, $bytes, 0x00675A, 4)
        [System.Array]::Copy($wB, 0, $bytes, 0x006784, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x00678D, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x0071EB, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x0071F4, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x007AE5, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x007AEE, 2)
        [System.Array]::Copy($hSub32B, 0, $bytes, 0x007B09, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x0080EC, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x0080F5, 2)
        [System.Array]::Copy($h32B, 0, $bytes, 0x008110, 4)
        [System.Array]::Copy($w32B, 0, $bytes, 0x008115, 4)
        [System.Array]::Copy($h32B, 0, $bytes, 0x00812B, 4)
        [System.Array]::Copy($w32B, 0, $bytes, 0x008130, 4)
        [System.Array]::Copy($w32B, 0, $bytes, 0x00816F, 4)
        [System.Array]::Copy($hB, 0, $bytes, 0x008193, 2)
        [System.Array]::Copy($w32B, 0, $bytes, 0x009DA0, 4)
        [System.Array]::Copy($h32B, 0, $bytes, 0x009DAC, 4)
        [System.Array]::Copy($wB, 0, $bytes, 0x009E06, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x009E0F, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x009EF9, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x009F02, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x014844, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x01484D, 2)

        # 2. Dynamic pitch patch at VA 0x00416FE9 (Raw 0x0163E7)
        # Replaces: mov dword ptr [0x004514AC], 640
        # With:     mov eax, [0x0045C01C]; mov [0x004514AC], eax (10 bytes exact)
        $pitchPatch = [byte[]]@(0xA1, 0x1C, 0xC0, 0x45, 0x00, 0xA3, 0xAC, 0x14, 0x45, 0x00)
        [System.Array]::Copy($pitchPatch, 0, $bytes, 0x0163E7, 10)
        [System.Array]::Copy($hSub32B, 0, $bytes, 0x0163F8, 2)

        # 3. In-game rendering clip bounds
        [System.Array]::Copy($wB, 0, $bytes, 0x01659A, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x0165A3, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x016608, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x016611, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x01663C, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x016645, 2)
        [System.Array]::Copy($wB, 0, $bytes, 0x017D30, 2)
        [System.Array]::Copy($hB, 0, $bytes, 0x017D39, 2)

        [System.IO.File]::WriteAllBytes($exePath, $bytes)
        return $true
    } catch {
        Write-Warning "Could not patch $exePath : $_"
        return $false
    }
}

function Get-CurrentSettings {
    $curW = 1280; $curH = 960; $curFS = $false
    if (Test-Path $IniPath) {
        $txt = [System.IO.File]::ReadAllText($IniPath)
        if ($txt -match '(?m)^width=(\d+)')      { $curW = [int]$matches[1] }
        if ($txt -match '(?m)^height=(\d+)')     { $curH = [int]$matches[1] }
        if ($txt -match '(?m)^fullscreen=(true|false)') { $curFS = ($matches[1] -eq 'true') }
    }
    return @{ W=$curW; H=$curH; FS=$curFS }
}

function Apply-Resolution([int]$W, [int]$H, [bool]$FS) {
    if (-not (Test-Path $IniPath)) {
        Write-Error "$IniPath not found!"
        return $false
    }

    $txt = [System.IO.File]::ReadAllText($IniPath)
    $txt = [regex]::new('(?m)^width=\d*').Replace($txt, "width=$W", 1)
    $txt = [regex]::new('(?m)^height=\d*').Replace($txt, "height=$H", 1)
    $fsVal = if ($FS) { "true" } else { "false" }
    $txt = [regex]::new('(?m)^fullscreen=(true|false)').Replace($txt, "fullscreen=$fsVal", 1)
    [System.IO.File]::WriteAllText($IniPath, $txt)

    $patched = 0
    if (Patch-ExecutableResolution $GameExe $W $H) { $patched++ }
    if (Test-Path $Win11Exe) {
        if (Patch-ExecutableResolution $Win11Exe $W $H) { $patched++ }
    }

    $modeStr = if ($FS) { "Borderless Fullscreen" } else { "Windowed" }
    Write-Host "`n[OK] Configured ${W}x${H} ($modeStr):" -ForegroundColor Green
    Write-Host "     - ddraw.ini updated"
    Write-Host "     - In-game viewport & camera engine patched ($patched executable(s))`n"
    return $true
}

function Start-BaldiesGame {
    if (Test-Path $GameExe) {
        Write-Host "`nLaunching Baldies..." -ForegroundColor Cyan
        Start-Process -FilePath $GameExe -WorkingDirectory $ScriptDir
    } elseif (Test-Path $Win11Exe) {
        Write-Host "`nLaunching Baldies (Win11)..." -ForegroundColor Cyan
        Start-Process -FilePath $Win11Exe -WorkingDirectory $ScriptDir
    } else {
        Write-Error "Game executable not found!"
    }
}

# Command-line parameter mode
if ($Width -gt 0 -and $Height -gt 0) {
    $fsBool = ($Fullscreen -in @("true", "1", "yes", "y", "t"))
    Apply-Resolution $Width $Height $fsBool
    exit 0
}

# Interactive Menu Mode
while ($true) {
    $cur = Get-CurrentSettings
    $curMode = if ($cur.FS) { "Borderless Fullscreen" } else { "Windowed" }

    Write-Host "==========================================================" -ForegroundColor Yellow
    Write-Host "          BALDIES - RESOLUTION & VIEW SELECTOR" -ForegroundColor White
    Write-Host "==========================================================" -ForegroundColor Yellow
    Write-Host " Current setting: $($cur.W) x $($cur.H) ($curMode)`n" -ForegroundColor Cyan
    Write-Host " Presets (Changes Window size + In-game Camera field of view):"

    foreach ($k in $Presets.Keys) {
        $p = $Presets[$k]
        $isActive = ($cur.W -eq $p.W -and $cur.H -eq $p.H -and $cur.FS -eq $p.FS)
        $actStr = if ($isActive) { " [ACTIVE]" } else { "" }
        $color = if ($isActive) { "Green" } else { "White" }
        Write-Host ("   [{0}]  {1,-12} - {2}{3}" -f $k, $p.Name, $p.Desc, $actStr) -ForegroundColor $color
    }

    Write-Host "`n   [9]  Custom Resolution (enter Width and Height)"
    Write-Host "   [L]  Launch Baldies now"
    Write-Host "   [Q]  Quit"
    Write-Host "==========================================================" -ForegroundColor Yellow

    $choice = (Read-Host "Enter your choice [0-9, L, Q]").Trim().ToUpper()

    if ($Presets.Contains($choice)) {
        $p = $Presets[$choice]
        Apply-Resolution $p.W $p.H $p.FS
        $ask = (Read-Host "Launch Baldies now? (Y/N)").Trim().ToUpper()
        if ($ask -eq "Y") {
            Start-BaldiesGame
            break
        }
    } elseif ($choice -eq "9") {
        try {
            $wIn = [int](Read-Host "Enter Width  (e.g. 1920)")
            $hIn = [int](Read-Host "Enter Height (e.g. 1080)")
            $fsIn = (Read-Host "Fullscreen? (y/n)").Trim().ToLower()
            $fsBool = ($fsIn -eq "y")
            Apply-Resolution $wIn $hIn $fsBool
            $ask = (Read-Host "Launch Baldies now? (Y/N)").Trim().ToUpper()
            if ($ask -eq "Y") {
                Start-BaldiesGame
                break
            }
        } catch {
            Write-Host "Invalid input, please enter valid numeric dimensions." -ForegroundColor Red
        }
    } elseif ($choice -eq "L") {
        Start-BaldiesGame
        break
    } elseif ($choice -eq "Q") {
        break
    } else {
        Write-Host "Invalid choice, please try again." -ForegroundColor Red
    }
}

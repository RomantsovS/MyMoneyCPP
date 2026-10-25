<#
.SYNOPSIS
    Smoke test of the Android back handling (ADR-0016, ADR-0019) on a phone or
    an emulator, driven with adb.

.DESCRIPTION
    Starts MyMoney, opens a menu, a sub-page, the Journal tab, the filter dialog
    and the operation form, goes back from each one and checks where the app
    lands; at the end two backs on Accounts must close the app. Nothing is
    saved or deleted: the app's data stays as it is.

    The checks read the app's accessibility tree (uiautomator dump): Qt Quick
    reports every control's text and its rectangle on the screen, so the test
    sees which page is really on screen - not only which tab is selected.

    Back is sent as a key event and, on a device with gesture navigation, also
    as a swipe from the left edge: both reach the app as Qt.Key_Back, but the
    gesture first touches the screen (see JournalFilter.qml).

    The device must be unlocked. Screenshots of failed checks are saved in
    build/android-back-test/.

.EXAMPLE
    tools\android-back-test.ps1 -Device emulator-5554

.EXAMPLE
    tools\android-back-test.ps1 -NoInstall -Back key
#>
[CmdletBinding()]
param(
    # An adb serial ("adb devices"); may be omitted when only one is attached.
    [string]$Device,
    # The APK to install; by default Qt Creator's debug build for the
    # device's ABI.
    [string]$Apk,
    # Test the installed app as it is.
    [switch]$NoInstall,
    # auto: the back key, and the back gesture where the device uses gestures.
    [ValidateSet('auto', 'key', 'gesture')]
    [string]$Back = 'auto'
)

Set-StrictMode -Version 3

$Package = 'io.github.romantsovs.mymoney'
$Activity = "$Package/org.qtproject.qt.android.bindings.QtActivity"
$Root = Split-Path -Parent $PSScriptRoot
$ShotDir = Join-Path $Root 'build\android-back-test'
$DumpPath = '/sdcard/mymoney-back-test.xml'
# The accounts page's menu button. (Windows PowerShell reads a script without
# a BOM as ANSI, so the script itself stays ASCII.)
$MenuGlyph = [string][char]0x22EE

function Find-Adb {
    foreach ($sdk in @($env:ANDROID_HOME, $env:ANDROID_SDK_ROOT, "$env:LOCALAPPDATA\Android\Sdk")) {
        if ($sdk -and (Test-Path "$sdk\platform-tools\adb.exe")) { return "$sdk\platform-tools\adb.exe" }
    }
    $onPath = Get-Command adb -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }
    throw 'adb not found: set ANDROID_HOME or put adb on PATH.'
}

function Invoke-Adb {
    & $script:Adb -s $script:Device @args
}

# The app's controls that are on screen now: text and rectangle. A control
# outside the screen (the other tab's page, a hidden page of a stack) has an
# empty or one-pixel rectangle and is left out.
function Get-Ui {
    for ($try = 0; $try -lt 3; $try++) {
        $null = Invoke-Adb shell uiautomator dump $DumpPath
        $raw = (Invoke-Adb exec-out cat $DumpPath) -join "`n"
        if ($raw -match '<hierarchy') { break }
        Start-Sleep -Milliseconds 300
    }
    $nodes = @()
    if ($raw -notmatch '<hierarchy') { return $nodes }
    foreach ($n in ([xml]$raw).SelectNodes('//node')) {
        if ($n.package -ne $Package) { continue }
        $text = if ($n.'content-desc') { $n.'content-desc' } else { $n.text }
        if (-not $text -or $n.bounds -notmatch '^\[(\d+),(\d+)\]\[(\d+),(\d+)\]$') { continue }
        $x1 = [int]$Matches[1]; $y1 = [int]$Matches[2]; $x2 = [int]$Matches[3]; $y2 = [int]$Matches[4]
        if ($x2 - $x1 -lt 20 -or $y2 - $y1 -lt 20) { continue }
        $nodes += [pscustomobject]@{ Text = $text; X1 = $x1; Y1 = $y1; X2 = $x2; Y2 = $y2 }
    }
    return $nodes
}

# A control with this text; -Top: in the title bar (a page title), -Bottom: in
# the tab bar.
function Find-Node($ui, [string]$text, [switch]$Top, [switch]$Bottom) {
    # $ui is $null when no control of the app is on screen.
    $ui | Where-Object {
        $null -ne $_ -and $_.Text -eq $text -and
        (-not $Top -or $_.Y1 -lt $script:Height * 0.15) -and
        (-not $Bottom -or $_.Y1 -gt $script:Height * 0.85)
    } | Select-Object -First 1
}

# Polls the screen until $condition (a script block of $ui) holds.
function Wait-Ui([scriptblock]$condition, [double]$seconds = 8) {
    $deadline = (Get-Date).AddSeconds($seconds)
    do {
        $ui = Get-Ui
        if (& $condition $ui) { return $true }
    } while ((Get-Date) -lt $deadline)
    return $false
}

function Invoke-Tap($node) {
    if (-not $node) { return }
    $x = [int](($node.X1 + $node.X2) / 2); $y = [int](($node.Y1 + $node.Y2) / 2)
    $null = Invoke-Adb shell input tap $x $y
}

function Invoke-TapText([string]$text, [switch]$Top, [switch]$Bottom) {
    $null = Wait-Ui { param($ui) $script:found = Find-Node $ui $text -Top:$Top -Bottom:$Bottom; [bool]$script:found } 5
    Invoke-Tap $script:found
}

# The back key, or the back gesture: a swipe from the left edge across the
# middle of the screen, as a thumb does it.
function Send-Back {
    if ($script:Mode -eq 'gesture') {
        $y = [int]($script:Height / 2)
        $null = Invoke-Adb shell input swipe 3 $y ([int]($script:Width * 0.55)) $y 250
    } else {
        $null = Invoke-Adb shell input keyevent KEYCODE_BACK
    }
}

function Test-Resumed {
    [bool]((Invoke-Adb shell dumpsys activity activities) -match "ResumedActivity.*$Package/")
}

function Test-Running {
    [bool]("$(Invoke-Adb shell pidof $Package)".Trim())
}

function Assert-Unlocked {
    $window = (Invoke-Adb shell dumpsys window) -join "`n"
    $power = (Invoke-Adb shell dumpsys power) -join "`n"
    if ($window -match 'isKeyguardShowing=true' -or $power -notmatch 'mWakefulness=Awake') {
        throw "The device $script:Device is locked or its screen is off: unlock it and run the test again."
    }
}

function Add-Result([string]$name, [bool]$ok) {
    $script:Results += [pscustomobject]@{ Mode = $script:Mode; Name = $name; Ok = $ok }
    if ($ok) {
        Write-Host "PASS  [$script:Mode] $name" -ForegroundColor Green
        return
    }
    $file = '{0}-{1:00}.png' -f $script:Mode, $script:Results.Count
    $null = New-Item -ItemType Directory -Force $ShotDir
    $null = Invoke-Adb shell screencap -p /sdcard/mymoney-back-test.png
    $null = Invoke-Adb pull /sdcard/mymoney-back-test.png (Join-Path $ShotDir $file) 2>$null
    Write-Host "FAIL  [$script:Mode] $name  (screenshot: build\android-back-test\$file)" -ForegroundColor Red
}

# Back to the starting point after a failed step, so the next one can run:
# the app restarted on Accounts.
function Restart-App {
    $null = Invoke-Adb shell am force-stop $Package
    $null = Invoke-Adb shell am start -W -n $Activity
    $ok = Wait-Ui { param($ui) [bool](Find-Node $ui 'Accounts' -Top) } 20
    if (-not $ok) { throw 'MyMoney did not show the Accounts page after the start.' }
}

function Test-Accounts($ui) {
    [bool](Find-Node $ui 'Accounts' -Top) -and -not (Find-Node $ui 'Journal' -Top)
}

function Open-Journal {
    Invoke-TapText 'Journal' -Bottom
    Wait-Ui { param($ui) [bool](Find-Node $ui 'Journal' -Top) -and -not (Find-Node $ui 'Accounts' -Top) }
}

function Invoke-Scenarios {
    Assert-Unlocked
    Restart-App

    # A popup menu closes; the page stays.
    Invoke-TapText $MenuGlyph -Top
    $opened = Wait-Ui { param($ui) [bool](Find-Node $ui 'Show archived') }
    Send-Back
    $ok = $opened -and (Wait-Ui { param($ui) -not (Find-Node $ui 'Show archived') -and (Test-Accounts $ui) }) -and (Test-Resumed)
    Add-Result 'menu: back closes the menu, Accounts stays' $ok
    if (-not $ok) { Restart-App }

    # A sub-page of a tab: one page back.
    Invoke-TapText $MenuGlyph -Top
    Invoke-TapText 'Categories'
    $opened = Wait-Ui { param($ui) [bool](Find-Node $ui 'Categories' -Top) }
    Send-Back
    $ok = $opened -and (Wait-Ui { param($ui) -not (Find-Node $ui 'Categories' -Top) -and (Test-Accounts $ui) }) -and (Test-Resumed)
    Add-Result 'sub-page: back returns from Categories to Accounts' $ok
    if (-not $ok) { Restart-App }

    # The Journal tab: back to Accounts - the page, not only the tab button.
    $opened = Open-Journal
    Send-Back
    $ok = $opened -and (Wait-Ui { param($ui) Test-Accounts $ui }) -and (Test-Resumed)
    Add-Result 'Journal tab: back shows the Accounts page' $ok
    if (-not $ok) { Restart-App }

    # A dialog: back closes only the dialog.
    $opened = Open-Journal
    Invoke-TapText 'Filter' -Top
    $opened = $opened -and (Wait-Ui { param($ui) [bool](Find-Node $ui 'Comment contains (all months)') })
    Send-Back
    $ok = $opened -and (Wait-Ui { param($ui) -not (Find-Node $ui 'Comment contains (all months)') -and (Find-Node $ui 'Journal' -Top) }) -and (Test-Resumed)
    Add-Result 'filter dialog: back closes only the dialog, Journal stays' $ok
    if ($ok) {
        Send-Back
        $null = Wait-Ui { param($ui) Test-Accounts $ui }
    } else {
        Restart-App
    }

    # The operation form opens the keyboard; the first back may only close it.
    $opened = Open-Journal
    Invoke-TapText '+'
    $opened = $opened -and (Wait-Ui { param($ui) [bool](Find-Node $ui 'New operation' -Top) })
    $left = $false
    for ($i = 0; $opened -and $i -lt 2 -and -not $left; $i++) {
        Send-Back
        $left = Wait-Ui { param($ui) -not (Find-Node $ui 'New operation' -Top) } 3
    }
    $ok = $left -and (Wait-Ui { param($ui) [bool](Find-Node $ui 'Journal' -Top) }) -and (Test-Resumed)
    Add-Result 'operation form: back returns to Journal' $ok
    if ($ok) {
        Send-Back
        $null = Wait-Ui { param($ui) Test-Accounts $ui }
    } else {
        Restart-App
    }

    # Accounts: one back only shows a hint.
    Send-Back
    $ok = (Wait-Ui { param($ui) [bool](Find-Node $ui 'Go back again to exit') } 3) -and (Test-Resumed)
    Add-Result 'Accounts: one back shows the hint, the app stays' $ok
    Start-Sleep -Milliseconds 2500  # the two seconds for a second back are over

    # Accounts: two backs close the app - the process ends.
    Send-Back
    Start-Sleep -Milliseconds 600
    Send-Back
    $deadline = (Get-Date).AddSeconds(5)
    while ((Test-Running) -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 300 }
    Add-Result 'Accounts: two backs close the app' (-not (Test-Running))
}

$Adb = Find-Adb
if (-not $Device) {
    $attached = @(& $Adb devices | Select-String '^(\S+)\s+device$' | ForEach-Object { $_.Matches[0].Groups[1].Value })
    if ($attached.Count -ne 1) { throw "Choose a device with -Device: $($attached -join ', ')" }
    $Device = $attached[0]
}
if ("$(& $Adb -s $Device get-state 2>$null)".Trim() -ne 'device') {
    throw "The device $Device is not attached (see adb devices)."
}

$size = @(Invoke-Adb shell wm size | Select-String '(\d+)x(\d+)')[-1].Matches[0]
$Width = [int]$size.Groups[1].Value
$Height = [int]$size.Groups[2].Value
$gestures = "$(Invoke-Adb shell settings get secure navigation_mode)".Trim() -eq '2'

$modes = switch ($Back) {
    'auto' { if ($gestures) { 'key', 'gesture' } else { 'key' } }
    'gesture' {
        if (-not $gestures) { throw "$Device uses button navigation: a back gesture would do nothing." }
        'gesture'
    }
    default { 'key' }
}

if (-not $NoInstall) {
    if (-not $Apk) {
        $abi = "$(Invoke-Adb shell getprop ro.product.cpu.abi)".Trim() -replace '-', '_'
        $Apk = Join-Path $Root "build\Qt_6_12_0_for_Android_${abi}_Debug\src\app\android-build-mymoney_app\mymoney_app.apk"
    }
    if (-not (Test-Path $Apk)) { throw "No APK at $Apk - build it in Qt Creator or pass -Apk." }
    Write-Host "Installing $Apk ($((Get-Item $Apk).LastWriteTime)) on $Device; the app's data is kept."
    # -r replaces the app and keeps its data.
    $null = Invoke-Adb install -r $Apk
    if ($LASTEXITCODE -ne 0) { throw 'adb install failed.' }
}

# The tree comes back as UTF-8; user data may hold any language.
$encoding = [Console]::OutputEncoding
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$Results = @()
try {
    foreach ($Mode in $modes) { Invoke-Scenarios }
} catch {
    # An error in the test itself (or a locked device) is not a pass.
    Write-Host "ERROR: $_" -ForegroundColor Red
    $Results += [pscustomobject]@{ Mode = $Mode; Name = 'error'; Ok = $false }
} finally {
    [Console]::OutputEncoding = $encoding
    $null = Invoke-Adb shell rm -f $DumpPath /sdcard/mymoney-back-test.png
}

$failed = @($Results | Where-Object { -not $_.Ok }).Count
Write-Host ''
Write-Host "$($Results.Count - $failed) passed, $failed failed on $Device ($($modes -join ' + '))."
exit [int]($failed -gt 0)

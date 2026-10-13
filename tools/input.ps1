# Real input into the game's window, as a user's: SendInput with scan codes (GLFW and raw input read those).
# Points are GD's: x from the left, y from the bottom, of a 569 x 320 screen (the width from -width).
#   input.ps1 move x y | down | up | rdown | rup | wheel n | key name [mods] | type text | shiftclick x y | drag x1 y1 x2 y2
param([string]$action, [string]$a = "", [string]$b = "", [string]$c = "", [string]$d = "", [double]$width = 569)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Input {
  [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx, dy; public uint data, flags, time; public IntPtr extra; }
  [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort vk, scan; public uint flags, time; public IntPtr extra; }
  [StructLayout(LayoutKind.Explicit)] public struct UNION { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
  [StructLayout(LayoutKind.Sequential)] public struct INPUT { public uint type; public UNION u; }
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
  [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr w, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr w, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr w);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
  public static void Mouse(uint flags, uint data) {
    var i = new INPUT[1]; i[0].type = 0; i[0].u.mi.flags = flags; i[0].u.mi.data = data;
    SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
  }
  public static void Key(ushort vk, bool up) {
    var i = new INPUT[1]; i[0].type = 1; i[0].u.ki.vk = vk; i[0].u.ki.scan = (ushort)MapVirtualKey(vk, 0);
    // KEYEVENTF_SCANCODE, and KEYEVENTF_EXTENDEDKEY for the arrows and the like
    uint flags = 0x0008;
    if (vk >= 0x21 && vk <= 0x2E) flags |= 0x0001;
    if (up) flags |= 0x0002;
    i[0].u.ki.flags = flags;
    SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
  }
}
"@

$game = Get-Process GeometryDash -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $game) { Write-Output "no game window"; exit 1 }
$window = $game.MainWindowHandle
[Input]::SetForegroundWindow($window) | Out-Null
Start-Sleep -Milliseconds 150

function To-Screen([double]$x, [double]$y) {
  $rect = New-Object Input+RECT
  [Input]::GetClientRect($window, [ref]$rect) | Out-Null
  $origin = New-Object Input+POINT
  [Input]::ClientToScreen($window, [ref]$origin) | Out-Null
  $w = $rect.Right - $rect.Left; $h = $rect.Bottom - $rect.Top
  return @([int]($origin.X + $x / $width * $w), [int]($origin.Y + (320 - $y) / 320 * $h))
}
function Move-Cursor([double]$x, [double]$y) { $p = To-Screen $x $y; [Input]::SetCursorPos($p[0], $p[1]) | Out-Null; Start-Sleep -Milliseconds 60 }

$keys = @{ d = 0x44; left = 0x25; up = 0x26; right = 0x27; down = 0x28; z = 0x5A; y = 0x59; c = 0x43; a = 0x41; h = 0x48; tab = 0x09; esc = 0x1B; f8 = 0x77; f1 = 0x70; pgup = 0x21; enter = 0x0D; slash = 0xBF; minus = 0xBD; backspace = 0x08; ctrl = 0xA2; shift = 0xA0; alt = 0xA4 }

switch ($action) {
  "move" { Move-Cursor ([double]$a) ([double]$b) }
  "down" { [Input]::Mouse(0x0002, 0); Start-Sleep -Milliseconds 80 }
  "up" { [Input]::Mouse(0x0004, 0); Start-Sleep -Milliseconds 80 }
  "rdown" { [Input]::Mouse(0x0008, 0); Start-Sleep -Milliseconds 80 }
  "rup" { [Input]::Mouse(0x0010, 0); Start-Sleep -Milliseconds 80 }
  "wheel" { for ($i = 0; $i -lt [Math]::Abs([int]$a); $i++) { [Input]::Mouse(0x0800, [uint32]([int]([Math]::Sign([int]$a) * 120) -band 0xFFFFFFFF)); Start-Sleep -Milliseconds 80 } }
  "ctrlwheel" {
    [Input]::Key($keys['ctrl'], $false); Start-Sleep -Milliseconds 60
    for ($i = 0; $i -lt [Math]::Abs([int]$a); $i++) { [Input]::Mouse(0x0800, [uint32]([int]([Math]::Sign([int]$a) * 120) -band 0xFFFFFFFF)); Start-Sleep -Milliseconds 80 }
    [Input]::Key($keys['ctrl'], $true); Start-Sleep -Milliseconds 80
  }
  "key" {
    $mods = @(); if ($b) { $mods = $b.Split("+") }
    foreach ($m in $mods) { [Input]::Key($keys[$m], $false); Start-Sleep -Milliseconds 30 }
    $code = $keys[$a]
    if (-not $code) { $code = [int][char]$a.ToUpper() }
    [Input]::Key($code, $false); Start-Sleep -Milliseconds 60; [Input]::Key($code, $true)
    foreach ($m in $mods) { Start-Sleep -Milliseconds 30; [Input]::Key($keys[$m], $true) }
    Start-Sleep -Milliseconds 100
  }
  "type" { foreach ($ch in $a.ToCharArray()) { $code = [int][char]([string]$ch).ToUpper(); if ($ch -eq '-') { $code = 0xBD }; [Input]::Key($code, $false); Start-Sleep -Milliseconds 40; [Input]::Key($code, $true); Start-Sleep -Milliseconds 40 } }
  "shiftclick" {
    Move-Cursor ([double]$a) ([double]$b); [Input]::Key($keys['shift'], $false); Start-Sleep -Milliseconds 60
    [Input]::Mouse(0x0002, 0); Start-Sleep -Milliseconds 80; [Input]::Mouse(0x0004, 0); Start-Sleep -Milliseconds 60
    [Input]::Key($keys['shift'], $true); Start-Sleep -Milliseconds 80
  }
  "drag" {
    Move-Cursor ([double]$a) ([double]$b); [Input]::Mouse(0x0002, 0); Start-Sleep -Milliseconds 100
    for ($i = 1; $i -le 10; $i++) { Move-Cursor ([double]$a + ([double]$c - [double]$a) * $i / 10) ([double]$b + ([double]$d - [double]$b) * $i / 10) }
    Start-Sleep -Milliseconds 100; [Input]::Mouse(0x0004, 0); Start-Sleep -Milliseconds 100
  }
}

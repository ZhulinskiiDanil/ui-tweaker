# Frames of the game's window for a GIF: -count shots, -every milliseconds apart, into -out as 000.png, 001.png...
param([string]$out, [int]$count = 40, [int]$every = 100)

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Frames {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr w, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr w, ref POINT p);
  public struct RECT { public int Left, Top, Right, Bottom; }
  public struct POINT { public int X, Y; }
}
"@
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$game = Get-Process GeometryDash -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $game) { Write-Output "no game window"; exit 1 }
$rect = New-Object Frames+RECT
[Frames]::GetClientRect($game.MainWindowHandle, [ref]$rect) | Out-Null
$origin = New-Object Frames+POINT
[Frames]::ClientToScreen($game.MainWindowHandle, [ref]$origin) | Out-Null
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
New-Item -ItemType Directory -Force $out | Out-Null
for ($i = 0; $i -lt $count; $i++) {
  $bitmap = New-Object System.Drawing.Bitmap $width, $height
  $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
  # The cursor isn't in a screen copy: drawn on top so the GIF shows what drags
  $graphics.CopyFromScreen($origin.X, $origin.Y, 0, 0, $bitmap.Size)
  $cursor = [System.Windows.Forms.Cursor]::Position
  $graphics.Dispose()
  $bitmap.Save((Join-Path $out ("{0:D3}.png" -f $i)), [System.Drawing.Imaging.ImageFormat]::Png)
  $bitmap.Dispose()
  Add-Content (Join-Path $out "cursor.txt") ("{0} {1}" -f ($cursor.X - $origin.X), ($cursor.Y - $origin.Y))
  Start-Sleep -Milliseconds $every
}
Write-Output "frames $count ($width x $height)"

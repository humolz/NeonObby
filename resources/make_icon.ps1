# Generates resources/icon.ico — multi-resolution neon "N" logo on dark circle.
# Uses System.Drawing (built into Windows) — no external tools required.

Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$outIco    = Join-Path $scriptDir 'icon.ico'

$sizes = @(16, 24, 32, 48, 64, 128, 256)
$pngEntries = @()

foreach ($size in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g   = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode   = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $g.Clear([System.Drawing.Color]::Transparent)

    # Dark navy circular background
    $bgBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 8, 12, 28))
    $g.FillEllipse($bgBrush, 0, 0, $size - 1, $size - 1)
    $bgBrush.Dispose()

    # Outer neon ring (cyan)
    $ringWidth = [Math]::Max(1, [int]($size * 0.06))
    $ringPad   = [int]($size * 0.05)
    $ringPen   = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 0, 220, 255), $ringWidth)
    $g.DrawEllipse($ringPen, $ringPad, $ringPad, $size - 2 * $ringPad - 1, $size - 2 * $ringPad - 1)
    $ringPen.Dispose()

    # Inner faint ring (magenta accent)
    if ($size -ge 32) {
        $accentPad = [int]($size * 0.14)
        $accentPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(120, 255, 60, 200), [Math]::Max(1, [int]($size * 0.025)))
        $g.DrawEllipse($accentPen, $accentPad, $accentPad, $size - 2 * $accentPad - 1, $size - 2 * $accentPad - 1)
        $accentPen.Dispose()
    }

    # Letter "N" in bright neon cyan
    $fontPx = [int]($size * 0.62)
    $font   = New-Object System.Drawing.Font('Arial Black', $fontPx, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $fmt    = New-Object System.Drawing.StringFormat
    $fmt.Alignment     = [System.Drawing.StringAlignment]::Center
    $fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
    $rect   = New-Object System.Drawing.RectangleF(0, 0, $size, $size)

    # Soft glow halo (draw letter several times in low alpha first)
    if ($size -ge 32) {
        $haloBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(80, 0, 255, 255))
        $g.DrawString('N', $font, $haloBrush, $rect, $fmt)
        $haloBrush.Dispose()
    }

    $letterBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 180, 250, 255))
    $g.DrawString('N', $font, $letterBrush, $rect, $fmt)
    $letterBrush.Dispose()
    $font.Dispose()
    $g.Dispose()

    # Encode as PNG
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    $pngEntries += [PSCustomObject]@{ Size = $size; Bytes = $ms.ToArray() }
    $ms.Dispose()
}

# Build the .ico container.
# ICO format: ICONDIR (6 bytes) + N * ICONDIRENTRY (16 bytes each) + image data.
# For sizes >= 64 we embed PNG-compressed images (modern Windows handles this).
$out = New-Object System.IO.MemoryStream
$bw  = New-Object System.IO.BinaryWriter($out)

# ICONDIR
$bw.Write([UInt16]0)               # reserved
$bw.Write([UInt16]1)               # type: 1 = ICO
$bw.Write([UInt16]$pngEntries.Count)

# ICONDIRENTRY headers
$dataOffset = 6 + (16 * $pngEntries.Count)
foreach ($entry in $pngEntries) {
    $w = if ($entry.Size -ge 256) { 0 } else { $entry.Size }
    $h = if ($entry.Size -ge 256) { 0 } else { $entry.Size }
    $bw.Write([Byte]$w)            # width  (0 means 256)
    $bw.Write([Byte]$h)            # height (0 means 256)
    $bw.Write([Byte]0)             # color count
    $bw.Write([Byte]0)             # reserved
    $bw.Write([UInt16]1)           # color planes
    $bw.Write([UInt16]32)          # bits per pixel
    $bw.Write([UInt32]$entry.Bytes.Length)
    $bw.Write([UInt32]$dataOffset)
    $dataOffset += $entry.Bytes.Length
}

# Image data
foreach ($entry in $pngEntries) {
    $bw.Write($entry.Bytes)
}

[System.IO.File]::WriteAllBytes($outIco, $out.ToArray())
$bw.Dispose()
$out.Dispose()

Write-Host ("Wrote {0} ({1:N0} bytes)" -f $outIco, (Get-Item $outIco).Length)

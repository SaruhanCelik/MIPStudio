param(
    [string]$Source = (Join-Path $PSScriptRoot '../assets/mips_studio_logo.png'),
    [string]$Destination = (Join-Path $PSScriptRoot '../assets/mips_studio.ico')
)

# Convert the supplied artwork, without changing its design, to a real Windows
# multi-resolution ICO. Merely renaming a PNG to .ico is not sufficient for RC.
Add-Type -AssemblyName System.Drawing
$sourceImage = [System.Drawing.Image]::FromFile((Resolve-Path -LiteralPath $Source))
$frames = [System.Collections.Generic.List[byte[]]]::new()
$sizes = @(16, 24, 32, 48, 64, 128, 256)
try {
    foreach ($size in $sizes) {
        $bitmap = [System.Drawing.Bitmap]::new($size, $size)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $stream = [IO.MemoryStream]::new()
        try {
            $graphics.Clear([System.Drawing.Color]::Transparent)
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $scale = [Math]::Min($size / $sourceImage.Width, $size / $sourceImage.Height)
            $width = [int][Math]::Round($sourceImage.Width * $scale)
            $height = [int][Math]::Round($sourceImage.Height * $scale)
            $rectangle = [System.Drawing.Rectangle]::new(
                [int](($size - $width) / 2), [int](($size - $height) / 2), $width, $height)
            $graphics.DrawImage($sourceImage, $rectangle)
            $bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
            $frames.Add($stream.ToArray())
        } finally {
            $stream.Dispose()
            $graphics.Dispose()
            $bitmap.Dispose()
        }
    }
} finally {
    $sourceImage.Dispose()
}

$output = [IO.File]::Create([IO.Path]::GetFullPath($Destination))
$writer = [IO.BinaryWriter]::new($output)
try {
    $writer.Write([uint16]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]$frames.Count)
    $frameOffset = 6 + 16 * $frames.Count
    for ($index = 0; $index -lt $frames.Count; ++$index) {
        $dimension = if ($sizes[$index] -eq 256) { 0 } else { $sizes[$index] }
        $writer.Write([byte]$dimension)
        $writer.Write([byte]$dimension)
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]32)
        $writer.Write([uint32]$frames[$index].Length)
        $writer.Write([uint32]$frameOffset)
        $frameOffset += $frames[$index].Length
    }
    foreach ($frame in $frames) {
        $writer.Write([byte[]]$frame)
    }
} finally {
    $writer.Dispose()
}
Write-Output "Generated $Destination with $($frames.Count) icon sizes."

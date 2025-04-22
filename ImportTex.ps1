 param (
    [switch]$srgb,
    [switch]$height,
    [switch]$singleChannel,
    [string]$filePath,
    [string]$width
 )

# Build texconv arguments
$texconvArgs = @($filePath, '-o', 'Assets', '-y', '-m', '5')

if (-not [string]::IsNullOrEmpty($width))
{
    $texconvArgs += @('-w', $width, '-h', $width)
}

if ($height)
{
    exiftool.exe -ColorSpace=Uncalibrated -overwrite_original $filePath
    texconv $texconvArgs -f R16_UNORM
}
elseif ($singleChannel)
{
    exiftool.exe -ColorSpace=Uncalibrated -overwrite_original $filePath
    texconv $texconvArgs -f BC4_UNORM
}
elseif($srgb)
{
    exiftool.exe -ColorSpace=sRGB -overwrite_original $filePath
    texconv $texconvArgs -f BC7_UNORM_SRGB
}
else
{
    exiftool.exe -ColorSpace=Uncalibrated -overwrite_original $filePath
    texconv $texconvArgs -f BC7_UNORM
}

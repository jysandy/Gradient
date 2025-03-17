 param (
    [switch]$srgb,
    [switch]$height,
    [switch]$singleChannel,
    [string]$filePath
 )

if ($height)
{
    exiftool.exe -ColorSpace=Uncalibrated -overwrite_original $filePath
    texconv $filePath -o Assets -y -m 5 -f R16_UNORM
}
elseif ($singleChannel)
{
    exiftool.exe -ColorSpace=Uncalibrated -overwrite_original $filePath
    texconv $filePath -o Assets -y -m 5 -f BC4_UNORM
}
elseif($srgb)
{
    exiftool.exe -ColorSpace=sRGB -overwrite_original $filePath
    texconv $filePath -o Assets -y -m 5 -f BC7_UNORM_SRGB
}
else
{
    exiftool.exe -ColorSpace=Uncalibrated -overwrite_original $filePath
    texconv $filePath -o Assets -y -m 5 -f BC7_UNORM
}

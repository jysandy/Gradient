 param (
    [switch]$srgb,
    [string]$filePath
 )

if($srgb)
{
    exiftool.exe -ColorSpace=sRGB -overwrite_original $filePath
}
else
{
    exiftool.exe -ColorSpace=Uncalibrated -overwrite_original $filePath
}

texconv $filePath -o Assets -y -w 2048 -h 2048 -m 5

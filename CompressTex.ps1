param (
	  [switch]$singleChannel,
	  [switch]$srgb,
	  [string]$filePath
)

if ($singleChannel)
{
	texconv $filePath -y -w 2048 -h 2048 -m 5 -f BC4_UNORM
}
elseif($srgb)
{
	texconv $filePath -y -w 2048 -h 2048 -m 5 -f BC7_UNORM_SRGB
}
else
{
	texconv $filePath -y -w 2048 -h 2048 -m 5 -f BC7_UNORM
}
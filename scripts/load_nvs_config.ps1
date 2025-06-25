param(
    [Parameter(Mandatory = $true)]
    [string]$port,

    [Parameter(Mandatory = $true)]
    [string]$size,

    [Parameter(Mandatory = $true)]
    [string]$offset
)

if (-not $env:IDF_PATH) {
    Write-Error "IDF environment is not configured"
    exit 1
}

$nvsConfigDir = Join-Path (Get-Location) "nvs_config"
$csv_file = Join-Path $nvsConfigDir "nvs_config.csv"
$bin_file = Join-Path $nvsConfigDir "nvs.bin"

$nvs_gen_script = Join-Path $env:IDF_PATH "components\nvs_flash\nvs_partition_generator\nvs_partition_gen.py"

$python = "python"

if (-not (Test-Path $csv_file)) {
    Write-Error "Nvs_config not found"
    exit 1
}

$gen_cmd = "$python `"$nvs_gen_script`" generate `"$csv_file`" `"$bin_file`" $size"
Invoke-Expression $gen_cmd

if ($LASTEXITCODE -ne 0) {
    exit 1
}

$flash_cmd = "$python -m esptool --port $port write_flash $offset `"$bin_file`""
Invoke-Expression $flash_cmd

# Формирует SQL-скрипт обновления манифеста SHA-256 по директории обновления.
# Пример: .\make_manifest.ps1 -Directory \\server\share\addin_2026 > manifest.sql
param(
    [Parameter(Mandatory = $true)]
    [string]$Directory
)

$root = (Resolve-Path $Directory).Path.TrimEnd('\') + '\'

$files = Get-ChildItem -Path $root -Recurse -File |
    Where-Object { $_.Extension -in '.dll', '.exe' }

"BEGIN TRANSACTION;"
"DELETE FROM [dbo].[addin_manifest];"
foreach ($file in $files) {
    $relative = $file.FullName.Substring($root.Length).Replace("'", "''")
    $hash = (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash
    "INSERT INTO [dbo].[addin_manifest] ([relative_path], [sha256]) VALUES (N'$relative', '$hash');"
}
"COMMIT;"

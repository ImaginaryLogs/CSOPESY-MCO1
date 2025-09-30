$ErrorActionPreference = 'Stop'

# Output dirs
New-Item -Force -ItemType Directory obj | Out-Null
New-Item -Force -ItemType Directory bin | Out-Null

# Collect sources
$srcs = Get-ChildItem -Recurse -File -Path src -Filter *.cpp | ForEach-Object { "`"$($_.FullName)`"" }
$SOURCES = $srcs -join ' '

# Flags
$CFLAGS = "/std:c++20 /EHsc /nologo /W4 /Zi /MP"
$OUTOBJ = "/Foobj\"
$OUTPDB = "/Fd:bin\app.pdb"
$OUTEXE = "/Fe:bin\app.exe"

Write-Host "Building with MSVC..."
# Use & to invoke cl.exe with a single argument string
cmd /c "cl $CFLAGS $OUTOBJ $OUTPDB $OUTEXE $SOURCES"
if ($LASTEXITCODE -ne 0) { throw "Build failed." }

Write-Host "`nBuild succeeded: bin\app.exe"

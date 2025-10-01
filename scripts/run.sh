#!/usr/bin/env bash
set -euo pipefail
if [[ ! -x bin/app ]]; then
  echo "bin/app not found. Run scripts/build.sh first."
  exit 1
fi
./bin/app

# Make the scripts executable:
# chmod +x scripts/build.sh scripts/run.sh
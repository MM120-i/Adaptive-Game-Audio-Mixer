set -euo pipefail

cd build
ctest --output-on-failure

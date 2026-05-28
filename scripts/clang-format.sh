#!/bin/bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_path="$(cd "$script_dir/.." && pwd)"

echo "Running clang-format in:"
echo "  $source_path"

find "$source_path" -maxdepth 1 \( -name '*.cpp' -o -name '*.h' \) | xargs clang-format -i

echo "Clang-format completed."

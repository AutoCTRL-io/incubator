#!/usr/bin/env bash

OUTPUT="project_output.txt"
SCRIPT_NAME="$(basename "$0")"

: > "$OUTPUT"  # clear output file

for file in *; do
  [[ -d "$file" ]] && continue
  [[ "$file" == "$SCRIPT_NAME" ]] && continue
  [[ "$file" == "instructions.txt" ]] && continue
  [[ "$file" == "README.md" ]] && continue
  [[ "$file" == "$OUTPUT" ]] && continue

  {
    echo "filename:"
    echo "$file"
    echo
    cat "$file"
    echo
  } >> "$OUTPUT"
done

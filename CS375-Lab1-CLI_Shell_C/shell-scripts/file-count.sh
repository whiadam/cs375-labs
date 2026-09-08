#!/bin/bash
read -p "Enter a directory: " DIR
if [ ! -d "$DIR" ]; then
    echo "Directory not found."
    exit 1
fi
echo "File counts in $DIR:" | tee file-count-log.txt
for ext in txt sh c; do
    count=$(find "$DIR" -type f -name "*.$ext" | wc -l)
    echo ".$ext files: $count" | tee -a file-count-log.txt
done
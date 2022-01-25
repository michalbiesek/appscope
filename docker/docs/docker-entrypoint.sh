#!/bin/bash

mkdir -p output

# for i in *.schema.json; do
#     [ -f "$i" ] || break
#     json-dereference -s "$i" -o output/"$i"
# done

# for i in output/*.schema.json; do
#     [ -f "$i" ] || break
#     json-schema-gendoc $i > ${i%%.*}.md
# done

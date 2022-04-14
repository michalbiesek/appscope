#!/bin/bash

symbol_file() {
    local lib_path=$1
    echo symbols_$(basename $lib_path .a).txt
}

extract_sym () {
    local lib_path=$1
    local output_file=$(symbol_file $lib_path)
    nm $lib_path | awk 'NF{print $NF}' | sort | uniq > $output_file
    sed -i '/_GLOBAL_OFFSET_TABLE_/d' $output_file
}

declare -a conrib_libs=("./contrib/build/ls-hpack/libls-hpack.a" 
                "./contrib/cJSON/libcjson.a" 
                "./contrib/build/funchook/libfunchook.a"
                "./contrib/build/funchook/capstone_src-prefix/src/capstone_src-build/libcapstone.a"
                "./contrib/build/libyaml/src/.libs/libyaml.a"
                "./contrib/build/openssl/libcrypto.a"
                "./contrib/build/openssl/libssl.a"
                "./contrib/build/pcre2/libpcre2-8.a"
                "./contrib/build/ls-hpack/libls-hpack.a"
)

libc_sym_path="./contrib/build/musl/lib/libc_orig.a"
sym_file=$(symbol_file "$libc_sym_path")

for lib in "${conrib_libs[@]}"
do
   extract_sym "$lib"
done

extract_sym "$libc_sym_path"
for lib in "${conrib_libs[@]}"
do
   contrib_file=$(symbol_file $lib)
   comm -12 "$contrib_file" "$sym_file"
done

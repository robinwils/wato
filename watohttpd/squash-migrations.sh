#! /bin/bash

migrations_commits() {
    git rev-list HEAD -- migrations/ | while read commit;do
        echo $(git log -1 $commit --pretty=format:'%h,%s,' --name-only)
    done
}

echo "squashed migrations from the following commits:" > summary.txt

migrations_commits | while IFS=, read -r hash subject files
do
    snapshot=false
    echo $files
    for file in $files; do
        [[ $file =~ ^.*collections.*\.go$ ]] && snapshot=true
    done
    if $snapshot; then
        echo "skipping collections snapshot in [$hash] $subject"
        continue
    fi
    echo "squashing migrations in [$hash] $subject"

    exist=false
    for file in $files; do
        [[ -f migrations/$(basename $file) ]] && exist=true && rm "migrations/$(basename $file)"
    done
    if $exist; then
        echo "[$hash] $subject" >> summary.txt
    fi
done

./watohttpd migrate collections

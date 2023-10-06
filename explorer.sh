#!/bin/bash

CLIMENU=""  # climenu path
SCRIPTPATH=$(readlink -f "$0")

if [ "$1" = "" ]
then
    EXPLOREPATH="$(pwd)"
else
    EXPLOREPATH="$(realpath "$1")"
fi

ENTRIES="
[Header]
str=$EXPLOREPATH

[Header]
str=

[Entry]
str=..
exec=$SCRIPTPATH $EXPLOREPATH/..
exit=true
"

for f in "$EXPLOREPATH"/*
do
    ENTRIES+="

[Entry]"

    if [ -f "$f" ]
    then
        ENTRIES+="
str=$(basename "$f")"
        ENTRIES+="
exec=less $f"
        ENTRIES+="
fgcolor=cyan"
    else
        ENTRIES+="
str=$(basename "$f")"
        ENTRIES+="
exec=$SCRIPTPATH $f"
        ENTRIES+="
fgcolor=green
exit=true
"
    fi
done

echo "$ENTRIES" | "$CLIMENU"
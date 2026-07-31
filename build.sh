set -e
gcc -o pipes *.c -Iinclude -lraylib -lm
./pipes

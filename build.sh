set -e
gcc -o pipes *.c -Iinclude -lraylib -lm -Wall -Wextra -Wpedantic
./pipes

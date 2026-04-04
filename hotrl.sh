inotifywait -m -r -e modify,create,delete --include '.*\.(cpp|hpp)$' ./root/module |
while read file; do
    cmake --build ./build --target module
done
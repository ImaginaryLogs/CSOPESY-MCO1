# Detect OS
UNAME_OUT="$(uname -s)"

if [[ "$UNAME_OUT" == "Linux" ]]; then

    g++ -std=c++23 -Wall src/*.cpp -o bin/app -lsfml-audio -lsfml-system
else
    g++ -std=c++23 -Wall src/*.cpp -o bin/app
fi
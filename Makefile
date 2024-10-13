all:
	gcc -I SDL2/include -L SDL2/lib -o SpaceInvaders SpaceInvaders.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf

run:
	SpaceInvaders.exe

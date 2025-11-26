CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fsanitize=address -lSDL3

test: notes_test.c note.c
	$(CC) $(CFLAGS) -o notes_test notes_test.c $(SDL_FLAGS) -lm
	./notes_test
	rm -f notes_test

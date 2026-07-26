NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread

SRC = src/main.c \
	  src/parsing.c \
	  src/time_utils.c \
<<<<<<< HEAD
	  src/heap.c
=======
	  src/heap.c \
	  src/init.c
>>>>>>> origin/master

OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.c includes/codexion.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

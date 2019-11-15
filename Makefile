#==============================================================
#======================== COMPILER ============================
#==============================================================
CC = gcc
CFLAGS = -Wall
OBJDIR = OBJS
EXEC = pdu

all:
	@echo "\033[33m"
	@echo "==============================="
	@echo "Building Source files"
	@echo "==============================="
	@echo "\033[0m"
	$(CC) $(CFLAGS) -I ./  -o $(EXEC) main.c pdu.c

.PHONY: clean
clean:
	@echo "\033[31m"
	@echo "==============================="
	@echo "Removing all files"
	@echo "==============================="
	@echo "\033[0m"
	@rm -rf $(OBJDIR)
	@rm -f *.o $(EXEC)

$(OBJDIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $(CFLAGS1) $< -o $@

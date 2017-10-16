CC = gcc
OBJS = bond_trigger.o

ALL: $(OBJS)
	echo "generated $(OBJS)"

$(OBJS): %.o:%.c
	$(CC) $(CFLAGS) -c $< -o $(OBJSDIR)/$@
	echo "compiled $(OBJS)"

.PHONY: clean
clean:
	$(RM) $(OBJSDIR)/$(OBJS)

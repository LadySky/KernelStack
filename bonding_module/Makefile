TARGET = Bond_Test
CC = gcc
OBJS = main.o
export RM = rm -rf
export INCLUDES = ${shell pwd}/include
export OBJSDIR = ${shell pwd}/objs
export CFLAGS = -I$(INCLUDES) -Wall -O2 -lpthread

$(TARGET): $(OBJSDIR) $(OBJS)
	$(MAKE) -C bond_module
	$(MAKE) -C bond_trigger
	$(MAKE) -C bond_parse
	$(MAKE) -C bond_diff
	$(MAKE) -C bond_link
	#$(MAKE) -C bond_actions
	$(CC) $(CFLAGS) $(OBJSDIR)/*.o -o $(TARGET)

$(OBJS): %.o:%.c
	$(CC) $(CFLAGS) -c $< -o $(OBJSDIR)/$@

$(OBJSDIR):
	mkdir -p $(OBJSDIR)

.PHONY: clean
clean:
	$(MAKE) -C bond_module clean
	$(MAKE) -C bond_trigger clean
	$(MAKE) -C bond_parse clean
	$(MAKE) -C bond_diff clean
	$(MAKE) -C bond_link clean
	#$(MAKE) -C bond_actions clean
	$(RM) $(Y)
	$(RM) $(TARGET)

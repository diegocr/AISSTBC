PROG	= AISSTBCREATOR
CC	= m68k-amigaos-gcc $(CPU) -noixemul
LIB	= -Wl,-Map,$@.map,--cref -lmui -lamiga
DEFINES	= 
WARNS	= -W -Wall -Winline
CFLAGS	= -O3 -funroll-loops -fomit-frame-pointer $(WARNS) $(DEFINES)
LDFLAGS	= -nostdlib
OBJDIR	= .objs$(SYS)
RM	= rm -frv

ifdef DEBUG
CFLAGS += -DDEBUG -g
endif

OBJS =	\
	$(OBJDIR)/startup.o	\
	$(OBJDIR)/swapstack.o	\
	$(OBJDIR)/main.o	\
	$(OBJDIR)/LayGroup.o	\
	$(OBJDIR)/utils.o	\
	$(OBJDIR)/CreateToolBar.o \
	$(OBJDIR)/ToolBar.o	\
	$(OBJDIR)/logo.o	\
	$(OBJDIR)/debug.o
#	$(OBJDIR)/masking.o	\

all:
#	make $(PROG)_060 SYS=_060 CPU="-m68060 -m68881"
	make $(PROG) CPU="-m68020-60 -msoft-float"

$(PROG)$(SYS): $(OBJDIR) $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIB)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
	@echo Compiling $@
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(PROG)_0*0 $(OBJDIR)_0*0


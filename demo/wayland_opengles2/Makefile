
BIN = demo

BINDIR = bin
OBJDIR = obj

# Flags
DEPS = xkbcommon wayland-client wayland-egl egl glesv2
CFLAGS += -std=c99 -pedantic -O2 $(shell pkg-config --cflags $(DEPS)) -I$(OBJDIR)

PROTOCOLS = xdg-shell relative-pointer-unstable-v1

PROTOCOL_DIR = $(shell readlink -f $$(pkg-config --variable=pkgdatadir wayland-protocols))
PROTOCOLS_H = $(foreach p,$(PROTOCOLS),$(OBJDIR)/$(p)-client-protocol.h)
PROTOCOLS_C = $(foreach p,$(PROTOCOLS),$(OBJDIR)/$(p)-protocol.c)

SRC = main.c
OBJS = $(addprefix $(OBJDIR)/,$(SRC:.c=.o)) $(PROTOCOLS_C:.c=.o)
EXE = $(BINDIR)/$(BIN)

LIBS = -lm $(shell pkg-config --libs $(DEPS))

.PHONY: clean

all: $(EXE)

$(EXE): $(OBJS) $(PROTOCOLS_H) | $(BINDIR)
	$(CC) $(OBJS) -o $(EXE) $(LIBS)

$(OBJDIR)/main.o: nuklear_wayland_gles2.h

$(OBJDIR)/%.o: %.c $(PROTOCOLS_H)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	$(RM) -r $(BINDIR) $(OBJDIR)

.SECONDEXPANSION:
protocol_xml = $(PROTOCOL_DIR)/$(if $(findstring unstable,$(1)),unstable/$(shell echo $(1) | sed -e 's/-unstable-v.*$$//'),stable/$(1))/$(1).xml

$(OBJDIR)/%-client-protocol.h: $$(call protocol_xml,%) | $(OBJDIR)
	wayland-scanner client-header $< $@

$(OBJDIR)/%-protocol.c: $$(call protocol_xml,%) | $(OBJDIR)
	wayland-scanner private-code $< $@

.SECONDARY:


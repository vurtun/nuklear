.PHONY : opengl x11 nanovg
all: opengl x11 nanovg
opengl:
	$(MAKE) -C opengl -f MakefileDebug
x11:
	$(MAKE) -C x11 -f MakefileDebug
nanovg:
	cd nanovg
	$(MAKE) -C nanovg -f MakefileDebug

ALL: rkmc rkms

rkmc: rkmc.c
	$(CC) -g -orkmc rkmc.c `pkg-config --cflags --libs glfw3 gl` -lm

rkms: rkms.c
	$(CC) -g -orkms rkms.c `pkg-config --cflags --libs x11 xtst` -lm

clean:
	-rm rkmc rkms

install: rkmc rkms
	install rkmc $(DESTDIR)/usr/local/bin/
	install rkms $(DESTDIR)/usr/local/bin/
	xdg-desktop-menu install --novendor ./com.github.jwillia3.rkmc.desktop
	xdg-desktop-menu install --novendor ./com.github.jwillia3.rkms.desktop

uninstall: rkmc rkms
	-rm $(DESTDIR)/usr/local/bin/rkmc
	-rm $(DESTDIR)/usr/local/bin/rkms
	xdg-desktop-menu uninstall ./com.github.jwillia3.rkmc.desktop
	xdg-desktop-menu uninstall ./com.github.jwillia3.rkms.desktop

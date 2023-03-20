rkm: rkm.c
	$(CC) -g -orkm rkm.c `pkg-config --cflags --libs x11 xtst`

test: rkm
	./rkm

clean:
	rm rkm

install: rkm
	install rkm $(DESTDIR)/usr/local/bin/
	xdg-desktop-menu install --novendor ./com.github.jwillia3.rkm.desktop

uninstall: rkm
	-rm $(DESTDIR)/usr/local/bin/rkm
	xdg-desktop-menu uninstall ./com.github.jwillia3.rkm.desktop

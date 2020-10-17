CFLAGS=-pipe -march=native -fomit-frame-pointer -mtune=native -Wno-psabi -g
CFLAGS+=-std=c++0x -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -DTARGET_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CMAKE_CONFIG -D__VIDEOCORE4__ -U_FORTIFY_SOURCE -Wall -DHAVE_OMXLIB -DUSE_EXTERNAL_FFMPEG  -DHAVE_LIBAVCODEC_AVCODEC_H -DHAVE_LIBAVUTIL_OPT_H -DHAVE_LIBAVUTIL_MEM_H -DHAVE_LIBAVUTIL_AVUTIL_H -DHAVE_LIBAVFORMAT_AVFORMAT_H -DHAVE_LIBAVFILTER_AVFILTER_H -DHAVE_LIBSWRESAMPLE_SWRESAMPLE_H -DOMX -DOMX_SKIP64BIT -ftree-vectorize -DUSE_EXTERNAL_OMX -DTARGET_RASPBERRY_PI -DUSE_EXTERNAL_LIBBCM_HOST

LDFLAGS=-L$(SDKSTAGE)/opt/vc/lib/
LDFLAGS+=-L./ -Lffmpeg_compiled/usr/local/lib/ -lc -lbcm_host -lomxil-bellagio -lz -lasound -ldvdread -lcairo

INCLUDES+=-I./ -Ilinux -Iffmpeg_compiled/usr/local/include/ -I /usr/include/dbus-1.0 -I /usr/lib/arm-linux-gnueabihf/dbus-1.0/include -I/usr/include/cairo -isystem$(SDKSTAGE)/opt/vc/include -isystem$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I/usr/lib/dbus-1.0/include

DIST ?= omxplayer-dist
STRIP ?= strip

SRC=	linux/XMemUtils.cpp \
		linux/OMXAlsa.cpp \
		utils/log.cpp \
		DynamicDll.cpp \
		utils/PCMRemap.cpp \
		utils/RegExp.cpp \
		BitstreamConverter.cpp \
		linux/RBP.cpp \
		OMXThread.cpp \
		OMXReader.cpp \
		OMXStreamInfo.cpp \
		OMXAudioCodecOMX.cpp \
		OMXCore.cpp \
		OMXVideo.cpp \
		OMXAudio.cpp \
		OMXClock.cpp \
		File.cpp \
		OMXPlayerVideo.cpp \
		OMXPlayerAudio.cpp \
		OMXPlayerSubtitles.cpp \
		SubtitleRenderer.cpp \
		DispmanxLayer.cpp \
		Srt.cpp \
		KeyConfig.cpp \
		OMXControl.cpp \
		Keyboard.cpp \
		omxplayer.cpp \
		AutoPlaylist.cpp \
		RecentFileStore.cpp \
		RecentDVDStore.cpp \
		OMXDvdPlayer.cpp \
		Subtitle.cpp \

OBJS+=$(filter %.o,$(SRC:.cpp=.o))

all: omxplayer.bin omxplayer.1

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

omxplayer.o: help.h keys.h

version:
	bash gen_version.sh > version.h 

omxplayer.bin: version $(OBJS)
	$(CXX) $(LDFLAGS) -o omxplayer.bin $(OBJS) -lvchiq_arm -lvchostif -lvcos -ldbus-1 -lrt -lpthread -lavutil -lavcodec -lavformat -lswscale -lswresample -lpcre
	$(STRIP) omxplayer.bin

help.h: README.md Makefile
	awk '/SYNOPSIS/{p=1;print;next} p&&/KEY BINDINGS/{p=0};p' $< \
	| sed -e '1,3 d' -e 's/^/"/' -e 's/$$/\\n"/' \
	> $@
keys.h: README.md Makefile
	awk '/KEY BINDINGS/{p=1;print;next} p&&/KEY CONFIG/{p=0};p' $< \
	| sed -e '1,3 d' -e 's/^/"/' -e 's/$$/\\n"/' \
	> $@

omxplayer.1: README.md
	sed -e '/This fork/,/sudo make install/ d; /DBUS/,$$ d' $< | sed -r 's/\[(.*?)\]\(http[^\)]*\)/\1/g' > MAN
	curl -F page=@MAN http://mantastic.herokuapp.com 2>/dev/null >$@

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	rm -f omxplayer.old.log omxplayer.log
	rm -f omxplayer.bin
	rm -rf $(DIST)
	rm -f omxplayer-dist.tgz
	rm -f version.h MAN omxplayer.1

.PHONY: ffmpeg
ffmpeg:
	@ INCLUDES='$(INCLUDES)' ./ffmpeg_helper.sh all

.PHONY: ffmpeg-download
ffmpeg-download:
	@ INCLUDES='$(INCLUDES)' ./ffmpeg_helper.sh download

.PHONY: ffmpeg-configure
ffmpeg-configure:
	@ INCLUDES='$(INCLUDES)' ./ffmpeg_helper.sh configure

.PHONY: ffmpeg-make
ffmpeg-make:
	@ INCLUDES='$(INCLUDES)' ./ffmpeg_helper.sh make

.PHONY: ffmpeg-install
ffmpeg-install:
	@ INCLUDES='$(INCLUDES)' ./ffmpeg_helper.sh install

.PHONY: ffmpeg-clean
ffmpeg-clean:
	@ ./ffmpeg_helper.sh clean

dist:
	tar -cPf $(DIST).tgz \
	--transform 's,^omxplayer$$,/usr/bin/omxplayer,S' \
	--transform 's,^omxplayer.bin$$,/usr/bin/omxplayer.bin,S' \
	--transform 's,^COPYING$$,/usr/share/doc/omxplayer/COPYING,S' \
	--transform 's,^README.md$$,/usr/share/doc/omxplayer/README,S' \
	--transform 's,^omxplayer.1$$,/usr/share/man/man1/omxplayer.1,S' \
	--transform 's,^ffmpeg_compiled/usr/local/lib/,/usr/lib/omxplayer/,S' \
	omxplayer omxplayer.bin COPYING README.md omxplayer.1 \
	`if [ -e ffmpeg_compiled ]; then echo ffmpeg_compiled/usr/local/lib/*.so*; fi`

install:
	cp omxplayer omxplayer.bin /usr/bin/
	mkdir -p /usr/share/doc/omxplayer
	cp COPYING /usr/share/doc/omxplayer/COPYING
	cp README.md /usr/share/doc/omxplayer/README
	cp omxplayer.1 /usr/share/man/man1
	if [ -e ffmpeg_compiled ]; then mkdir /usr/lib/omxplayer && cp -P ffmpeg_compiled/usr/local/lib/*.so* /usr/lib/omxplayer/; fi

uninstall:
	rm -rf /usr/bin/omxplayer
	rm -rf /usr/bin/omxplayer.bin
	rm -rf /usr/lib/omxplayer
	rm -rf /usr/share/doc/omxplayer
	rm -rf /usr/share/man/man1/omxplayer.1

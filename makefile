wifi_sound: main.cpp recorder.cpp port.cpp tool.cpp log.c
#	../../mips-openwrt-linux-g++ -std=c++0x -pthread -I/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/include -L/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib main.cpp recorder.cpp port.cpp tool.cpp log.c -o wifi_sound -pthread -lasound -lspeex -Wno-write-strings
	g++ -std=c++0x -pthread main.cpp recorder.cpp port.cpp tool.cpp log.c -o wifi_sound -pthread -lasound -lspeex -Wno-write-strings

clean:
	rm wifi_sound

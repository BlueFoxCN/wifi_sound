wifi_sound: enc_c.cpp
	../../mips-openwrt-linux-g++ -std=c++0x -pthread -I/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/include -L/mnt/vdc/openwrt/attitude_adjustment/staging_dir/target-mips_r2_uClibc-0.9.33.2/usr/lib enc_c.cpp -o wifi_sound -lasound -lspeex -Wno-write-strings

clean:
	rm wifi_sound

# Server-FS-app

compilation process :
    -add Simconnect.lib in C:\Program Files\mingw-w64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\x86_64-w64-mingw32\lib and rename it libsimconnect.a
    -write the command : $ g++ server_full_thread.cpp simconnect.cpp -o compiled1 -I ./include -lpthread -lws2_32 -lwsock32 -lsimconnect

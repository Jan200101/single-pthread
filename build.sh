CFLAGS="-fomit-frame-pointer -g -Wall -I. -lrt $@"

gcc ${CFLAGS} main.c pthread.c


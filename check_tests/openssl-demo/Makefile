ALL:
	gcc -mrdrnd -Wall -Wextra -O2 -o aesctr aesctr.c -lcrypto
clean:
	-rm aesctr
run:
	./aesctr </dev/zero | head -c1G | sha1sum


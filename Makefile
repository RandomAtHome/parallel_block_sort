TARGETS=block_sort
MCC=mpicc
CFLAGS=-O3 -Wall -pedantic

$(TARGETS): main.o
	${MCC} -o $@ $<

%.o: %.c
	${MCC} ${CFLAGS} -c $<

.PHONY: clean
clean:
	rm -f ./*.o $(TARGETS) ./*.vg.log

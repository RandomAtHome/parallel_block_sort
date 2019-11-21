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

reset_table:
	echo "NetSize	NumberCount	RSeed	UsedBitsCount	PreGeneration	Generation	SendingSortSignal	TotalSortTime	OrderCheckTime	TotalRunTime	NumbersDistribution" > sort_results.tsv

show_table:
	column -t sort_results.tsv

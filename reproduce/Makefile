all:
	nvcc -ccbin g++-10 -arch=sm_70 -O2 tc_bug.cu -o tc_bug -lcudart

clean:
	rm -f tc_bug

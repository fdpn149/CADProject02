all: set_lib_path mlrcs

set_lib_path: gurobi.tar.gz
	tar -zxvf gurobi.tar.gz

mlrcs: lib.h node.h node.cpp manager.h manager.cpp B11015061.cpp
	g++ -o mlrcs B11015061.cpp manager.cpp node.cpp -I./include -L./lib -lgurobi_c++ -lgurobi100 -lm -m64 -O3

clean: lib include lib.h node.h node.cpp manager.h manager.cpp B11015061.cpp
	rm -r lib include

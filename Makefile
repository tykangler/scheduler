RL_OPT = -std=c++17 -c
DB_OPT = -std=c++17 -c -g
DB_EXE = -std=c++17 -g

OUT_TESTS = -o bin/tests/
OUT_BUILD = -o build/

OBJ_TGTS = ThreadPool.o Scheduler.o WorkStealingQueue.o
OBJ_PATHS = build/ThreadPool.o build/Scheduler.o build/WorkStealingQueue.o
TESTS = schedulertest exectest graphtest queuetest

SRC_PAR = src/Parallel/
SRC_CIP = src/Cipher/
SRC_ACC = src/Accurate/

tests: $(TESTS)
	g++ tests/schedulertest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)schedulertest
	g++ tests/exectest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)exectest
	g++ tests/graphtest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)graphtest
	g++ tests/randtest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)randtest
	g++ tests/scramblertest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)scramblertest
	g++ tests/opstest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)opstest
	g++ tests/hashtest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)hashtest
	g++ tests/queuetest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)queuetest

schedulertest: tests/schedulertest.cpp $(OBJ_TGTS)
	g++ tests/schedulertest.cpp $(OBJ_PATHS) $(DB_EXE) $(OUT_TESTS)$@

exectests: tests/exectests.cpp 
	g++ tests/exectests.cpp $(DB_EXE) $(OUT_TESTS)$@

graphtest: tests/graphtest.cpp $(OBJ_TGTS)
	g++ tests/graphtest.cpp $(DB_EXE) $(OUT_TESTS)$@

scramblertest: tests/scramblertest.cpp
	g++ tests/scramblertest.cpp $(DB_EXE) $(OUT_TESTS)$@

queuetest: tests/queuetest.cpp WorkStealingQueue.o
	g++ tests/queuetest.cpp build/WorkStealingQueue.o $(DB_EXE) $(OUT_TESTS)$@

tasktests: tests/tasktests.cpp
	g++ tests/tasktests.cpp $(DB_EXE) $(OUT_TESTS)$@

ThreadPool.o: $(SRC_PAR)ThreadPool.cpp $(SRC_PAR)ThreadPool.hpp
	g++ $(SRC_PAR)ThreadPool.cpp $(DB_OPT) $(OUT_BUILD)$@

Scheduler.o: $(SRC_PAR)Scheduler.cpp $(SRC_PAR)Scheduler.hpp
	g++ $(SRC_PAR)Scheduler.cpp $(DB_OPT) $(OUT_BUILD)$@

WorkStealingQueue.o: $(SRC_PAR)WorkStealingQueue.hpp $(SRC_PAR)WorkStealingQueue.hpp
	g++ $(SRC_PAR)WorkStealingQueue.cpp $(DB_OPT) $(OUT_BUILD)$@
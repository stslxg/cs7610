FLAGS = -std=c++11 -pthread
DEPS = common.h
OBJS = main.o utils.o pack.o heap.o

%.o: %.cpp %.h $(DEPS)
	g++ $(FLAGS) -c -o $@ $<
main: $(OBJS)
	g++ $(FLAGS) -o prj1 $(OBJS)
clean:
	rm prj1 *.o

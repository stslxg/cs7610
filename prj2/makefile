FLAGS = -std=c++17 -pthread -latomic
DEPS = common.h
OBJS = main.o utils.o pack.o

%.o: %.cpp %.h $(DEPS)
	g++ $(FLAGS) -c -o $@ $<
main: $(OBJS)
	g++ $(FLAGS) -o prj2 $(OBJS)
clean:
	rm prj2 *.o

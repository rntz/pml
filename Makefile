CXX=g++
CXXFLAGS= -Wall -Wextra -Werror

SRCS=gc sched rt
INCS=$(addsuffix .hpp,$(SRCS))
OBJS=$(addsuffix .o,$(SRCS))

%.o: %.cpp $(INCS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

libpml.a: $(OBJS)
	rm -f $@
	ar qsc $@ $^

.PHONY: clean
clean:
	rm -f libpml.a $(OBJS)

CXX=g++
CXXFLAGS= -Wall -Wextra -Werror

SRCS=gc sched rt
INCS=$(addsuffix .hpp,$(SRCS))
OBJS=$(addsuffix .o,$(SRCS))

libpml.a: $(OBJS)
	rm -f $@
	ar qsc $@ $^

example: example.cpp libpml.a
	$(CXX) $^ -o $@

$(OBJS): %.o: %.cpp $(INCS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f example libpml.a $(OBJS)

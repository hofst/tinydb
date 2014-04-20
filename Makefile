DEPTRACKING=-MD -MF $(@:.o=.d)
# CXXFLAGS:=-g -std=c++11 -Wall -Wextra -Isrc
# BUILDEXE=g++ -o$@ $(CXXFLAGS) $(LDFLAGS) $^

CXXFLAGS:=-g -stdlib=libc++ -std=c++11 -Wall -Wextra -Isrc
BUILDEXE=clang++ -o$@ $(CXXFLAGS) $(LDFLAGS) $^

CHECKDIR=@mkdir -p $(dir $@)
EXEEXT:=

all: bin/admin$(EXEEXT) examples_bin

include src/LocalMakefile
include examples/LocalMakefile

-include bin/*.d bin/*/*.d 

bin/%.o: src/%.cpp
	$(CHECKDIR)
	clang++ -o$@ -c $(CXXFLAGS) $(DEPTRACKING) $<

bin/examples/%.o: examples/%.cpp
	$(CHECKDIR)
	clang++ -o$@ -c $(CXXFLAGS) $(DEPTRACKING) $<

clean:
	find bin -name '*.d' -delete -o -name '*.o' -delete -o '(' -perm -u=x '!' -type d ')' -delete

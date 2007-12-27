# ***** Variables

labelSources = TermTree.cpp Strategy.cpp PrintDebugStrategy.cpp	\
  FrobeniusStrategy.cpp BenchmarkStrategy.cpp			\
  DecompositionStrategy.cpp StatisticsStrategy.cpp		\
  CompositeStrategy.cpp PrintProgressStrategy.cpp		\
  SkipRedundantStrategy.cpp LabelAlgorithm.cpp OldPartition.cpp

rawSources = $(patsubst %, label/%, $(labelSources))			\
  main.cpp Action.cpp							\
  IrreducibleDecomAction.cpp fplllIO.cpp io.cpp fourti2.cpp		\
  randomDataGenerators.cpp monosIO.cpp BigIdeal.cpp FormatAction.cpp	\
  macaulay2IO.cpp newMonosIO.cpp HelpAction.cpp				\
  stdinc.cpp DynamicFrobeniusAction.cpp dynamicFrobeniusAlgorithm.cpp	\
  GenerateIdealAction.cpp GenerateFrobeniusAction.cpp			\
  IrreducibleDecomFacade.cpp FrobeniusAction.cpp Facade.cpp		\
  IOFacade.cpp DynamicFrobeniusFacade.cpp RandomDataFacade.cpp		\
  AnalyzeAction.cpp IdealFacade.cpp Parameter.cpp			\
  ParameterGroup.cpp GenerateIdealParameters.cpp IntegerParameter.cpp	\
  IrreducibleDecomParameters.cpp BoolParameter.cpp			\
  Lexer.cpp Partition.cpp StringParameter.cpp Term.cpp			\
  TermTranslator.cpp Timer.cpp VarNames.cpp LatticeFormatAction.cpp	\
  SliceAlgorithm.cpp							\
  Ideal.cpp intersect.cpp IntersectFacade.cpp IntersectAction.cpp	\
  AssociatedPrimesFacade.cpp AssociatedPrimesAction.cpp			\
  PrimaryDecomAction.cpp Slice.cpp					\
  IndependenceSplitter.cpp Projection.cpp DecomWriter.cpp		\
  SliceStrategy.cpp lattice.cpp						\
  LatticeFacade.cpp PrimaryDecomFacade.cpp DecomRecorder.cpp		\
  TermGrader.cpp

ldflags = -lgmpxx -lgmp
cflags = -Wall -ansi -pedantic -Wextra -Wno-uninitialized	\
         -Wno-unused-parameter -Werror 

ifndef MODE
 MODE=release
endif

ifeq ($(MODE), release)
  outdir = bin/release/
  cflags += -O3
else ifeq ($(MODE), debug)
  outdir = bin/debug/
  cflags += -g -D DEBUG -fno-inline
else ifeq ($(MODE), profile)
  outdir = bin/profile/
  cflags += -g -pg -O3
  ldflags += -pg
else ifeq ($(MODE), analysis)
  outdir = bin/analysis/
  cflags += -fsyntax-only -O1 -Wfloat-equal -Wundef			\
            -Wno-endif-labels -Wshadow -Wlarger-than-1000		\
            -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings	\
            -Wconversion -Wsign-compare -Waggregate-return		\
            -Wmissing-noreturn -Wmissing-format-attribute		\
            -Wno-multichar -Wno-deprecated-declarations -Wpacked	\
            -Wredundant-decls -Wunreachable-code -Winline		\
            -Wno-invalid-offsetof -Winvalid-pch -Wlong-long		\
            -Wdisabled-optimization
else
  $(error Unknown value of MODE: "$(MODE)")
endif

sources = $(patsubst %.cpp, src/%.cpp, $(rawSources))
objs    = $(patsubst %.cpp, $(outdir)%.o, $(rawSources))
program = frobby.exe

# ***** Compilation

.PHONY: all depend clean bin/$(program)
all: bin/$(program) $(outdir)$(program)
ifeq ($(MODE), profile)
	rm -f gmon.out
	./bench
	gprof ./frobby > prof
endif

test: all
ifdef TESTCASE
	export frobby=bin/$(program); echo; echo -n "$(TESTCASE): " ; \
	cd test/$(TESTCASE); ./runtests $(TESTARGS); cd ../..
else
	export frobby=bin/$(program); ./test/runtests
endif

bench: all
	cd data;time ./runbench

$(outdir): $(outdir)label
	mkdir -p $(outdir)
$(outdir)label:
	mkdir -p $(outdir)label


# Make symbolic link to program from bin/
bin/$(program): $(outdir)$(program)
ifneq ($(MODE), analysis)
	cd bin; cp -sf ../$(outdir)$(program) $(program); cd ..
endif

# Link object files into executable
$(outdir)$(program): $(objs) | $(outdir)
ifneq ($(MODE), analysis)
	g++ $(objs) $(ldflags) -o $(outdir)$(program)
endif
ifeq ($(MODE), release)
	strip bin/$(program)
endif


# Compile and output object files.
# In analysis mode no file is created, so create one
# to allow dependency analysis to work.
$(outdir)%.o: src/%.cpp | $(outdir)
	  g++ ${cflags} -c $< -o $(outdir)$(subst src/,,$(<:.cpp=.o))
ifeq ($(MODE), analysis)
	  echo > $(outdir)$(subst src/,,$(<:.cpp=.o))
endif

# ***** Dependency management
depend:
	g++ -MM $(sources) | sed 's/^[^\ ]/$$(outdir)&/' > .depend
-include .depend

clean: tidy
	rm -rf bin frobby_v*.tar.gz

tidy:
	find -name "*~" -exec rm -f {} \;
	find -name "*.stackdump" -exec rm -f {} \;
	find -name "gmon.out" -exec rm -f {} \;
	find -name "*.orig" -exec rm -f {} \;
	find -name "core" -exec rm -f {} \;

# ***** Mercurial

commit: test
	echo
	hg commit -m "$(MSG)"

# ***** Distribution

distribution:
	make depend
	make fastdistribution VER="$(VER)"

fastdistribution: tidy
	cd ..;tar --create --file=frobby_v$(ver).tar.gz frobby/ --gzip \
	  --exclude=*/data/* --exclude=*/data \
	  --exclude=*/.hg/* --exclude=*/.hg --exclude=.hgignore\
	  --exclude=*/bin/* --exclude=*/bin \
	  --exclude=*/save/* --exclude=*/save \
	  --exclude=*/4ti2/* \
	  --exclude=*.tar.gz \
      --exclude=*/web/* \
      --exclude=*/web \
      --exclude=*/tmp/* \
      --exclude=pullf \
      --exclude=pushf \
      --exclude=sync \
      --exclude=*~ --exclude=*.orig \
      --exclude=gmon.out \
      --exclude=*.stackdump
	mv ../frobby_v$(VER).tar.gz .
	ls -l frobby_v$(VER).tar.gz

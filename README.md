# WIP: Fuzzing Program Slices

This is experimental code with many areas that could be improved.
Much is hacked together and may require adjustment because of 
hardcoded paths (sorry).


Update: since this was a WIP, there is some updated work along this
path. It will be placed into https://github.com/roachspray/FuzzingProgramSlices
e.g., the exitblocks code there is still using naive analysis phase, basing
modification on execution flow from an input sample, but inserts exit(0).


## Introduction

The goal of this research is to investigate fuzzing program slices.
The idea behind this is to be able to target certain sub-graphs of
a program for fuzz testing, avoiding paths you are not interested
in at the time. There are many possible routes to take in doing
this, all with some similar issues; namely that any removal of 
compare/branch pairs will possibly result in the need to fix any
crash samples generated on the slice so that they can cause the
original, non-sliced program to reach the same/similar state.
This is not necessarily a trivial problem. Also realize that the
goal of the slice generation is to have it be compiled and executable
within the same input parameter space as the trace run described
below.

The high level methodology being investigate is the following:

```
  A <- Analyze(P)
  S <- Slice(P, A)
  {C'_i} <- Fuzz(S)
  CrashesP <- {}
  for each c' in {C'_i}
      c <- MapCrash(P, S, c')
      if c can crash P
          CrashesP.add(c) 
```

Of course there is much left open here. Also should note that an
ideal Slice()+MapCrash() pairing would result in all crashes mapping.
A perfect Slice() would result in MapCrash() being the identity map 
for the incoming crash input to be mapped.

One such possibility for the above is the following:

```
  P_t <- instrumentForTracing(P)   // Some level of dynamic analysis
  T <- exec(P_t, I)
  P_d <- slice(P, T, d)   // Slice generated with trace log as analysis input
  F_d <- instrumentForFuzzing(P_d, AFL) 
  {C_d,i} <- fuzz(F_d, I, AFL)
  for each c in {C_d,i}
      if exec(P, c) results in crash:
          Note(c)
      else:
          c_P <- fuzz(c, P)
          if exec(P, c_P) results in crash:
              Note(c_P)
 
```

In words: you first instrument the original program so you can 
generate a trace file based on execution with some input. This
trace file is used as an input to the slicing algorithm along with
a depth, d. The depth is to indicate the limit of call depth on the
pruning / modification of the program***. The slice is generated and
then instrumented+compiled for the purposes of fuzzing. Fuzzing occurs
on the instrumented slice and, hopefully?, generates a set of input
samples that will crash the slice. Next, one goes through each 
sample and determines if it can crash the original program as-is, or
if it can be ``mapped'' to a sample that will crash the original.


***Currently only execution trace and depth are used... I investigated
using SVF for static value flow analysis, but was unable to modify to
the point of making what I wanted to have happen..happen..and have not
implemented my own.  There is also using libdft or manticore since I'm
already doing dynamic. This is a good thought and to be done as an
experiment. See the nextsteps.md (MAKE LINK)


The goal is that the slices are generated in such a way that either

- Samples causing slice to crash will crash original program, or

- Samples causing slice to crash are trivially mapped to sample that will crash original program, or

- Samples causing slice to crash may require non-trivial changes to crash original, but
the time taken to fuzz slice and map sample is less than fuzzing original program to find the crash.

So there are, in effect, two conjectures being investigated:

1. There exists a slicing algorithm that works on all programs that satisfies the above 3 conditions. 

2. There exists a slicing algorithm that works on some known subset of programs, with identifiable 
properties, that satisfies the above 3 conditions.


Either way, saying the slice will generate sample inputs, c_slice, that
maps to a input c_orig reasonably implies that there is some measure in which

```
   d(c_slice) - d(c_orig) <= Something reasonable :)
```

Or that the probability of c_slice mappable to c_orig is non-trivial.


### Trace generation 

The current trace mechanism is quite hokey. At every basic block, a call is
inserted to invoke a function from a library, libtrays. The idea is that libtrays
could be whatever you want, but the way it is now, it blindly printf()'s to a 
file. No locking, nothing. It's quite simple. That being said, what is generated
is what is used by the slicing algorithm...So changing things would require some
work.

This all works on LLVM IR level. The LabelMDInject pass is run and at every basic
block injects some metadata that includes function name, block index, and block
address, if it is available. Injecting as metadata will allow us to perform slicing
and still make references to the same (or slightly modified) blocks. The TraceBBInject
pass then uses this information to add calls to libtrays. 

The modified IR is compiled to a binary and executed with an input file that should
reach areas of the program graph, in execution, that you are interested in fuzzing.
So, if you were looking at tcpdump and they added a protocol NEWPROTO, you'd probably
wish to graph a PCAP file that has NEWPROTO traffic so the trace will reach that parsing
code.

The output looks something like

```
1 main 1 424fff
2 env_options 1 4b7e86
2 env_options 4 4b7f93
2 process_options 1 4b5b68
2 process_options 4 4b5bde
2 process_options 5 4b5c22
2 process_options 6 4b5c46
2 process_options 7 4b5c7a
```

So this is depth, function, block index, address. We cannot always get address as the
method used for that is an LLVM one which will not always emit the address.


### Slicing as it is now

I do not claim this is good, but it is what it is now. And is a first step. I would
greatly appreciate help to improve this.

Based on the trace log, blocks are split up into keep and remove. The kept blocks are
those blocks that are reached (i.e., they have a log entry) and/or are below the 
maximum slicing depth specified. Those removed are not reached and are in functions
executed and below the maximum slicing depth.

For each of the blocks that are marked to be removed, the goal of the algorithm is
to attempt to remove it as a possible successor in each of it's predecessor blocks. 

There is currently no analysis to determine impact on other blocks. That is, by 
removing a block, you could implicitly be removing the possible use of other blocks.
To reduce program on-disk and in-memory size, one could attempt to determine these
blocks and remove them.

See the [PruneHelp](https://github.com/roachspray/acsac17wip/blob/master/naive/src/Transform/PruneHelp.cpp)  and [PruneBlocks](https://github.com/roachspray/acsac17wip/blob/master/naive/src/Transform/PruneBlocks.cpp) code for this naive method. 

```
  for d in maxdepth:1:0:
      setFocusFunction(d)
      if focusFunction == null:
          continue
      for rb in removeBlock:
          for pred in predecessors(rb):
             TerminatorInst t = getTerminator
             /* attempt to remove rb from predecessor */
```
 
         
### Mapping Crashes

Currently all mapping of crashes is done by attempting to use found crashes as
inputs to fuzzing the original program. This is under the thinking that such a 
smple could be, under some measure, not far from a sample that will crash the
original and that the fuzzer will make that leap. Not very good :-) But kind of
interesting to flesh out. I simply take the original bitcode and instrument
with afl-fast-clang and let it rip

There are a few other ideas for this that are more involved. They are investigated
in the next steps.md and are/or should be listed as open issues. They include
using iterated fuzzing steps, modified fuzzer, and CSP solving.


### Next steps

Clearly this is a WIP. The next steps are described in the 
[nextsteps.md file](https://github.com/roachspray/pruneskin/blob/master/doc/nextsteps.md)
and are each quite involved. 

## Using what is here

### System

I have only really tested this under Ubuntu 16.something. It will work with
clang/llvm 3.8, 3.9, 4.0.. Specify which in the Makefile. 

Just run make from the top level.

### Running

The pruneskin code works on IR and so you will need to get the target
code into bitcode file form. I suggest a few methods for doing this, 
if going after a random project:

- Try wllvm.. See the [Makefile.wllvm](https://github.com/roachspray/pruneskin/blob/master/test/tcpdump/Makefile.wllvm) file for an example of how to make that go.

- If that won't work, I often will run "make -n" on the target's Makefile or adjust SHELL
variable. Then you can take that output and modify to generate a build script that will
-emit-llvm and then use llvm-link to link those bitcode files together. 

There are a couple of methods for running the code, including a python script, a
set of shell scripts, and using opt command itself. Currently, I suggest the python
script found in pyhelpers/. The shell scripts in helpers/ are possibly out of date.

#### Running with pskin_help.py

The key here is that you have a a target .bc file and any other bits (libraries) that
you might need to compile that to an executable. You will then want to setup a json
file that describes your project. For example, see the tcpdump arp.json in test 
directory. It is like the following:

```
{
  "out_dir" : "/home/areiter/pruneskin/test/tcpdump/work_arp",
  "ps_root" : "/home/areiter/pruneskin",
  "bc_file" : "tcpdump.bc",
  "compile_lib" : ["-lcrypto", "-lssl", "-lpcap"],
  "fnlist_file" : "probecallfunctions.txt",
  "depths" : [  6, 7, 8  ],
  "command" : "-r @@"
}
```

Specifies a working dir (out_dir), the pruneskin root path, the target file, 
possible libraries, functions to not prune return value compare-branches (see
above discussion on slice generation), call depths to maximally prune through,
and how AFL might be run (though that part is not implemented). You can put other
values in this json file, too... as from the [pskin/__init__.py code](https://github.com/roachspray/pruneskin/blob/master/pyhelpers/pskin/__init__py):

```
pskin_vars = {}
pskin_vars['opt'] = '/usr/lib/llvm-4.0/bin/opt'
pskin_vars['clang'] = '/usr/lib/llvm-4.0/bin/clang'
pskin_vars['clang++'] = '/usr/lib/llvm-4.0/bin/clang++'
pskin_vars['afl_fuzz'] = '/usr/local/bin/afl-fuzz'
pskin_vars['afl_clang'] = '/usr/local/bin/afl-clang-fast'
pskin_vars['afl_clang++'] = '/usr/local/bin/afl-clang++-fast'
pskin_vars['objdump'] = '/usr/bin/objdump'

# Updated by modules and stuff
required_keys = [ "opt",
  "clang", 
  "clang++",
  "ps_root",
  "lib_trays",
  "lib_trace",
  "lib_prune",
  "afl_fuzz",
  "afl_clang",
  "afl_clang++",
  "objdump",
  "command",
  "bc_file",
  "depths",
  "fnlist_file",
  "out_dir"
]
#optional so far are:
#  compile_lib which should be a n array for [-L/path -llib... type] 
#    compilation requirements
```

So, you can see what you might change for your setup.

Once you have prepared your target's json file. You can run pskin_help.py -t target.json. 
This will create the working directory and generate a trace version of your target.
You want to run this executable on the target input sample that will reach the area
of code you are interested in fuzzing. It will generate a trace.log file and will
be used to generate the slice, as discussed above. 

For better or worse, tracing doesn't like having a trace.log file there already, so
beware if re-running. You could take two traces and join them... increasing the 
input information to the slice generation.

So now that you have done that you want to generate slices, and slices that are
AFL-able. You can just run pskin_help.py -s target.json and in the work directory you 
will have this.  You can now fuzz the depN_afl executables with AFL.

If wishing to take your target.bc and instrumenting with AFL, you can use the 
afl-clang-fast tool. Just be sure to specify AFL_CC/AFL_CXX to the correct versions
of clang/LLVM you have used for the target.bc generation.etc


# If need more info

If need/want more info, let me know and I am happy to help. I also encourage
people to help with more general issues or telling me it sucks :)


# Acknowledgments 

- Jared Carlson (@jcarlson23) for support and encouragement with ideas and as
my LLVM mentor.

- Veracode research for supporting this work.

-

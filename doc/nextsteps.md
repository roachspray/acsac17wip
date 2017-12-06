
There are a bunch of things that are in need of being worked on. 

- Improve controls and testing. Currently, things are in the realm of
pseudo-scientific method. This includes contrived tests and known tests (i.e. reproduce known CVEs). One issue, for old CVEs, is the lack of data about time til finding the crash.

- Investigate a feedback signal instead of pruning. That is, modify what might be 
 removed code, replacing it with a signal or other feedback that AFL/other can
 treat as a flag to avoid that path.

- Investigate bootstrap cascading. So, find crash in slice of depth N, bootstrap with that
against slice of depth N-1, take results, bootstrap to N-2, ... to original.


- Investigate using the slice crashes as input to original that is only selectively
instrumented with AFL.
See [https://github.com/roachspray/afl/tree/selective_instrumentation](https://github.com/roachspray/afl/tree/selective_instrumentation)
This selection would be generated from trace file that is used for slice generation.

- Investigate selective instrumentation only. 

- Investigate using libdft or manticore to runtime trace dataflow for slicing input.

- Investigate another method for static analyis value flow  as SVF was not playing nicely with 
what I was needing.

- Investigate using manticore to solve intermediary differences with crash inputs. This would
be instead of using slice crashes to bootstrap AFL, you use symbolic execution to attempt
to reach the crash state against the original. At each branch location of difference, try to
solve CSP that gets you to the next execution step that coincides with the crash path.
See [Manticore](https://github.com/trailofbits/manticore)


- Investigate crash replay with manticore


- Investigate independent fuzzing of whole code paths. So the idea is

```
  Take program P... split it up somehow into independent parts
  P_1, P_2, .., P_N

  The idea is to fuzz the pieces independently, somehow, and then link
  states. So each P_i results in good and bad states, conceivably. The
  idea would be to have Good(P_1) /\ Good(P_2) /\ Bad(P_3) so a crash 
  occurs in P_3 ... if you can reconstruct input cause these states, then
  you are in good shape.
```


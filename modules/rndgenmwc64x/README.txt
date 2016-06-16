David Thomas
dt10@imperial.ac.uk
http://www.doc.ic.ac.uk/~dt10/

---------------------------------------------------------

This package is version 0 of the MWC64X random
number generator for OpenCL. It implements a
high quality uniform random number generator
which only requires two 32-bit words of state and five
or six instructions per sample, and so is ideal for
use in GPUs.

The quality has been checked empirically: specifically it
passes every test from BigCrush in TestU01, both in it's
standard and bit-reversed form (i.e. the LSBs are just
as good as the MSBs), and a chi2 test of uniformity over
2^[4..20] buckets over 2^40 consecutive samples. Basically
it passes every reasonable (or unreasonable) test I can find,
and if you find something it fails, I'd love to know.

Note that the period is 2^63, so in  principle you can
exhaust the entire period if you run simulations for
a very long time, or on many GPUs in parallel. You should
never use randomised seeding, instead use the provided
skipping code, which splits the sequence into non-overlapping
sequences of a given size. For example, if each thread
will consume at most 2^40 samples (about an hours run
at a very optimisitic 1GSamples/sec), then you still have
2^23 distinct streams.

-----------------------------------------------------------




Edits by Daniel Jönsson:
Added OpenCL parallel seed generator (mwc64xseedgenerator and cl/randstategen.cl). 


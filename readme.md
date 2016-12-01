HPCE 2016 CW6
=============

This coursework is due on:

    Fri Dec 9th, 22:00

Submission is via github, and as before the
two people in pairs should connect together
their repositories via write permisions.

Auto-runs
=========

The schedule I'm planning for auto-runs is:

- Fri 2nd, 15:00

- Mon 5th, 18:00

- Tue 6th, 18:00

- Wed 7th, 18:00

- Thu 8th, 18:00

This is mainly dictated by when I have time to nurse the instances.

I _may_ also run them at other times (probably Thu 1st, as I'm debugging
the flow with real submissions).

Note that these are the times when I will pull - the actual
results will take a while to churn through, particularly
as there are two instance types to spin up. My estimated
lag is four hours, but that isn't guaranteed.

Also, the same "dt10_runs/count_me_in" thing applies if you
want to see comparative results.

Overall Goals
=============

The overall goal for this coursework is to explore
methods of accelerating a (simplified) version of
the POETS simulator. As this is rather an open-ended
task, I'm looking for two things:

- (60%) Concrete speed-up of the given simulator when using
  a g2.2xlarge _or_ a c4.4xlarge. Speed-up here is defined
  as the total execution time of the simulator, from
  program start time to finish time. The particular application
  I'm interested in is the "heat" application, which is very
  similar to the one from CW3.

- (40%) Insight, analysis, and suggestions for good strategies
  for accelerating the simulator of POETS applications.
  You have limited time to actually implement ideas, so
  this is a chance to explain what you would do if you
  were me.

Context and Background
======================

POETS is a research project ([funded by the EPSRC](http://gow.epsrc.ac.uk/NGBOViewGrant.aspx?GrantRef=EP/N031768/1))
that is attempting to build asynchronous hardware platforms
in order to provide high-performance compute systems. A
key underlying idea is that ordered communication channels
and synchronous barriers make it difficult to scale to
10K+ cores. So the idea of POETS is to decompose applications
into 100K+ asynchronous "devices" - they are roughly
analogous to agents or threads - which then communicate via
the passing of small messages. Compared with current systems
we plan to have very fine-grain compute and communication,
with each device containing 10-100 bytes of state, and
each message containing 4-10 bytes of payload. The messages
passed between devices flow over the edges of a graph,
which reflects the underlying topology of the application.

In order to build this system, we have to rethink a number
of components in a computer system:

- Hardware: the underlying POETS hardware consists of
  ~10K RISC-V cores, all connected together via a custom
  interconnect. The system has to be able to efficiently
  handle billions of tiny packets per second, so both
  the CPUs and network must tightly coupled, and optimised
  around message passing.

- Systems software: a traditional operating system does
  not think in terms of graphs, so a custom OS is needed
  which understands the topology of the hardware, and
  can then map a given application topology onto the
  hardware. For a regular 2D toplology (e.g. the cartesian
  grid seen in CW3) this is relatively easy, but most
  problems require less regular or completely
  unstructured meshes.

- Applications: Any application that will be run on POETS
  has to be completely rewritten or re-thought in order
  to decompose it into devices and messages. As there are
  no barriers or ordering, it is necessary to build any
  synchronisation into the messages themselves. However,
  reintroducing synchronisation will limit performance, so
  the difficult part is designing applications which are
  globally asynchronous, but still give the right answer.

The project began only recently, so neither the hardware
nor systems software exist yet. This presents a problem
for the applications people (Imperial + Newcastle), as
we have to try to develop and test applications without
having a way of running them. We can simulate the applications,
but this leads to a lot of slow-down.

So there are two key things we currently need from
simulators:

- High performance: we are interested in seeing whether
  applications still work for very large problem graphs,
  but this means we have to simulate a fine-grain message
  passing machine on a traditional machine. Simulating
  ~1e6 messages/sec is fairly easy, but the real
  hardware will be supporting ~1e10 messages/sec. Closing
  the performance gap gives more confidence that the
  applications being developed will actually scale well.

- Hardware fidelity: the fastest way of simulating POETS
  is to completely ignore the network, and deliver messages
  as soon as they are generated, but in the real world
  there will be a network, and it will get congested.
  While it is possible to design applications that can
  tolerate any amount of delay, they will still slow down.
  Simulating some amount of network delay gives more
  confidence, but also makes the simulation process much
  slower.

More detailed specifiations
===========================

What I want from you is something that allows us to explore
and evaluate possible POETS applications faster, and with
a better idea about performance. The simulator I've given
you is cut-down on functionality, but contains most of the
same features in terms of complexity.

So things you need to deliver are:

- A (correct) simulator which supports the "heat" application. This
  should be compilable by calling:

      make user_simulator

  in the submission root directory.

    - The resulting output executable should be called `bin/user/simulator`,
      and should support the same parameters, inputs, and outputs as `bin/ref/simulator`.

    - One thing the simulator produces is a stream of statistics, explaining
      what the devices and edges were doing in each hardware "time step". This
      should match the output of the reference simulator.

    - The simulator should also produce a stream which is the "output" of
      the application. So this is the answer we would be interested in if
      we ran it on the real hardware. This is motion-JPEG compressed,
      so I will accept _small_ RMSE distortions, for an unspecified but
      "reasonable" level of small (this gives me some wiggle room, and stops
      over-optimisation on the less interesting stuff).

    - The simulator also produces a stream of statistics, which describes
      what the simulated hardware is doing in terms of simulated
      CPU and network activity.

- A file called `readme.pdf` which covers two areas:

    - An explanation of how your current code and approach works, which allows
      an interested reader to then look at and be able to follow the code.

    - A discussion of general strategies and parallelisation methods
      that can be applied, including their strengths and weaknesses.
      I'm particularly interested in strategies for GPU-based acceleration,
      though multi-core (and multi-machine) is good too. These can be
      methods that you've invented, or methods you've seen other
      people discuss, but you need to explain why they would be
      a good choice in this context. Also don't try to cover every
      single idea - just think of the highest impact or most
      interesting approaches.

  Your readme.pdf does not need to be incredibly long or highly
  polished, but it should try to clearly explain your ideas and
  reasoning. I would imagine around two pages per section is
  enough. Figures are useful, but they don't need to be highly
  detailed. Hand-drawn figures are completely fine as long as they
  are understandable (and don't result in 50MB pdfs...)

The default target platform is g2.2xlarge. If you want to target
the c4.4xlarge instead, then create a file called "c4.4xlarge" in
your root directory.

You can modify anything in your submission directory, as long
as you are willing to handle merge conflicts if the spec is
updated. I would suggest not changing things in the following
directories:

- `include`

- `src/ref`

- `src/tools`

- `dt10_runs`

Problem overview
================

The idea of POETS is to support many applications, but in order
to make this manageable I'm getting you to focus on just one,
which is basic heat-diffusion. This is not actually a very
realistic application, as one would never actually solve it
in the way we'll do it, but it is simple enough to be tractable
in the time we have, while also exposing most of the performance
problems.

The heat application
--------------------

The idea of the heat simulation is to discretise space
into multiple devices, each representing the heat of
a spatial cell, and then evolve the spread of heat
across a number of discrete time-steps. We want
to evolve the heat solution forwards for the whole graph,
so each point does that by running the following process:

1 - Broadcasting its initial heat to all neighbouring devices

2 - Wait to receive the current heat from all neighbouring devices

3 - Calculating its next heat based on the current heat of itself,
    and broadcast to its neighbours.

4 - Go back to step 2.

Some of the cells are [dirichlet](https://en.wikipedia.org/wiki/Dirichlet_boundary_condition))
boundary conditions, and calculate their current heat using a fixed
function of time. To keep things simple, the boundary condition function
is really simple, though it is still non-linear (which makes it
a problem that can't be solved analytically).

The devices are all completely asynchronous, so they
have to maintain a local shared notion of which timestep
they are on. What complicates things is that you may
receive updates from neighbours before you have actually
sent yours, so each device may be active in two timesteps
at the same time. This leads to the wavefront evolution
of "time" shown in the lecture, where different parts of
the graph can be at very different simulation times, while
local parts are more constrained.

The neighbourhoods of the devices are defined by channels,
which are directional edges between pairs of devices. Each
channel has a weight, which defines how easily heat flows
along that path (in the real application this is based
on the local spatial and temporal derivatives of the PDE).
For this application there are three kinds of interesting
channel configurations:

- "rect" : the devices are connected in a regular 2D mesh,
  with north, south, east and west neighbours.

- "hex" : the devices are connected into a 2D honeycomb
  lattice, so interior cells have 6 neighbours.

- "mesh" : the connectivity of each cell can be different,
  with no particular structure.

Different problems tend to lead to different topologies,
so finite-difference methods tend to use "rect", "hex"
can sometimes be useful for approximating non
axis-aligned forces, while "mesh" is most useful where
the world has sharp curves and corners.

Each device steps through every time point during the
solution, which means the "full" solution consists of
the value of every device at every time point. However,
if we have 1M devices, and run it for 1M time-steps
(on the lower end of things), this generates approximately
4TB of data. While there is a huge amount of bandwidth
between devices, getting the data out is a much
bigger problem, so the actual output result must be
sub-sampled in space and time. For example, we might
choose to observe only 1/8 of the devices, and only
capture every 100th time-step, reducing the bandwidth
to 5GB, which is much more manageable and can then
be streamed to a single machine for visualisation.

Responsibility for capturing and managing this data
is given to the "supervisor" device, which runs on
a normal CPU. In the real architecture there are many
of these supervisor CPUs, but we'll only model one
here. Because the devices are all operating at different
time-points, the supervisor has to build up "slices"
of time. Each output message will be associated with
a particular simulation slice, so it must be inserted
into the appropriate slice buffer. Once the buffer
is full it can be interpolated for visualisation, then
sent to the output. In this example, the slices are rendered
using nearest neighbour interpolation, then sent to
a file as MJPEG (a sequence of JPEGs).

So the job of the supervisor is:

1. For each output message (h,t):

   a. If this is the first output for slice t, allocate a new buffer

   b. Insert the heat h into the correct position in the buffer based on the source device

2. If the earliest time-slice is complete (all values received) then:

   a. Spatially interpolate the mising values in the heat solution

   b. Encode the resulting heat-map as JPEG and write to file

   c. Discard the time-slice

The result of running the devices and supervisor together is
a graph of the heat over time, which is the "solution".

The POETS simulator
-------------------

The heat application only describes the behaviour of the
application, so in order to execute it we need some
kind of hardware. For a given POETS application we would
like to run it on different simulators, as well as the
target hardware, including:

- Functional simulators: these try to run the application
  logic as fast as possible, in order to test the high-level
  properties, i.e. does it calculate the right answer?

- Behavioral simulators: as well as running the application
  logic, these try to capture some elements of how the
  underlying functional units of the hardware work in order
  to give insight into how fast it is likely to run.

- Cycle-accurate simulators: every element of the hardware
  is captured at the cycle-level, including all bus-level
  transactions. This is incredibly slow, but ensures that
  all low-level performance effects are captured.

The tradeoff between simulation speed and simulation fidelity
is very common - consider a functional versus timing-driven
simulation of digital circuits. I'm going to get you to
look at a behavioural simulator, which captures some elements
of the underlying hardware, but extracts others away.

The specific assumptions I'll use are:

- Each device is mapped to one node, containing a CPU.

- Each channel is mapped to one network edge.

- The simulated hardware proceeds in "steps", where
  each step represents on discrete unit of hardware time.

- At each step, every node gets a chance to send a
  message, and place the message in all outgoing network
  edges.

- At each step, every edge gets a chance to deliver
  a message to its target node.

- Each network edge can contain at most one message.

- Each network edge has a delay, which captures the
  length of time it takes the message to transit the
  edge.

- A node can only send a message if all outgoing
  edges are empty.

There is a [simple graphical simulator](https://poetsii.github.io/graph_schema/heat/test.html)
of these semantics available here, although it doesn't
model network delays.

So during each step a node can be in one of the
following states:

- idle : the device on the node didn't want to send a message,
  so there is nothing to do at the application level.

- blocked : the device wanted to send a message at the
  application level, but there was no network capacity
  at the hardware level.

- sent : the application generated a message

Similarly the edges can be in a number of states:

- idle : no messages arrived or were sent

- transit : there is a message in the edge, but it is
  still in flight

- delivered : a message exited the edge and was
  received by the application-level device.

During each time step the simulator produces a summary
of statistics, counting how many nodes and devices were
in each state. The idea is that this can be used to find
patterns and problems in the hardware utilisation.

_*Note*: While simplifying the network model to turn this
into the coursework, I took out a source of randomness applied
to the delay on a per-packet basis (simulating hardware uncertainty).
At the time I thought that the network delays still resulted in
unpredictable statistics, but I was only looking at big graphs.
For smaller graphs there is clear periodicity after a while,
related to the largest network delay and/or the LCM of the
delays._

_Detecting and exploiting periodicity is not a useful solution
in this scenario, as it is only an artefact of the simplification.
It is tempting to add back in the randomness, but that would
cause a breaking-change, and people are already working on
their code and have generated reference outputs. So far I haven't
seen anyone trying to exploit the periodicity, so I'm simply going
to state: *detecting statistical periodicity is not a valid
solution, and should not be applied*._

Running the simulator
=====================

In order to run the simulator, you need an input file.

First build the tools:

    make reference_tools

You may need to `sudo apt install libjpeg-dev` in order to get the
JPEG compression libraries and headers.

Then generate a graph:

    bin/tools/generate_heat_rect 5

You'll see the structure of a 5x5 graph printed out.
Save the output to a file:

    mkdir -p w
    bin/tools/generate_heat_rect 5 > w/heat5.graph

Then you can simulate it:

    bin/ref/simulator w/heat5.graph - w/heat5.mjpeg

This will produce:

- Statistics printed to stdout (the hardware utilisation)

- The application output printed to w/heat5.mjpeg as a motion jpeg

- Logging information printed to stderr

Let's generate higher resolution 63x63 mesh, run it for
1024 heat updates, and sub-sample by 4 spatially and 8 temporally,
and turn the logging off:

    mkdir -p w
    bin/tools/generate_heat_rect 63 1024 4 8  > w/heat63.graph
    bin/ref/simulator --log-level 0 w/heat63.graph - w/heat63.mjpeg

It will run for a while, and then finish, with a longer
heat trace and more statistics. Even though we only
wanted 1024 application level time points, it has taken
over 17000 hardware steps to achieve it, due to network
delays on some edges.

If you want to look at the output, then tools that can I believe
can play mjpeg are:

- ffplay or avplay from ffmpeg/avlib. Cross-platform.

- [Media Player Classic](https://mpc-hc.org/), Windows.

*Note: looking at the mjpeg output can be useful to determine what the
calculation is doing, and for large-scale breakages. However, if you're checking
correctness you want to automate it and not rely on human eyes. One
approach is to ensure the outputs are diffable (binary exact). Another
approach is to find an image diff program. Another approach is to notice
that the jpeg_helper contains a [decompressor](https://github.com/HPCE/hpce-2016-cw6/blob/master/include/jpeg_helpers.hpp#L62)
as well as a compressor.*

In terms of performance, the metric of performance for
this scenario is simply the execution time of the simulator:

    time bin/ref/simulator --log-level 0 w/heat63.graph w/heat63.stats w/heat63.mjpeg

The goal is to minimise the "heat" simulation time for:

- Different numbers of nodes

- The three different topology types (rect, hex, mesh)

- Different levels of spatial and temporal sub-sampling of the output.
  The more nodes there are, the longer it takes to get large-scale
  heat movements, so sub-sampling (especially temporal) tends to
  increase as the problem gets bigger.

By nature POETS is supposed to support many applications,
which is why the simulator is written in such a strange
way. However, to make the problem more tractable, I'm only
asking you to look at "heat".

Managing expectations
=====================

This is not an artifically constructed task where I know exactly
how to "solve" it. It is a simplified port of an existing simulator
I wrote, with some features taken out. It is also quite
a complicated piece of code, so there is a possibility that
we'll encounter bugs which I'll need to patch. It may also
be slightly under-specified, as it wasn't constructed solely
with assesment in mind - if refinements are needed, let me know.

I obviously have a lot of ideas about good
ways of doing it, and already have a very fast multi-core
functional simulator. There is also a large amount of existing research
into how to make this kind of problem faster and more parallel, but
not this specific problem. There are some
parts of the problem where I have knowingly made things sub-optimal,
mainly by deliberately not pre-optimising things as I wrote it,
but there are not that many.

In terms of GPU acceleration, my assumption is that this can be
made to work quite well on a GPU. The question is how to map
the different memory structures to that level, and how to deal
with the output side of things.

Overall, I expect to see quite large speed-ups, but you
will need to transform the code quite a bit to get those.
Note that you are free to strip away abstraction layers
if you wish - the separation between hardware simulator
and application is important for generic code, but you
know that the only target of interest here is heat.

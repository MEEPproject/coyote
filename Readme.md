# Coyote #

Coyote is an execution-driven simulator based on the open source RISC-V ISA that is in development as part of MEEP. Coyote focuses on data movement and the modelling of the memory hierarchy of the system, which is one of the main hurdles for high performance and efficiency, while omitting lower level details. As a result, it provides sufficient simulation throughput and fidelity to enable first order comparisons of very different design points within reasonable simulation time.

To leverage previous community efforts, Coyote is built upon two preexisting simulation tools: Spike and Sparta. Spike offers the functional simulation and L1 cache modelling capabilities, and has also been extended with some extra features, such as RAW dependency tracking and instruction latencies. Sparta has been used to model the memory hierarchy starting from the L2. The following figure provides a very high level representation of the kinds of architectures that Coyote targets.

![Example of an architecture targeted by Coyote](Coyote/docs/images/Coyote_overview.png?raw=true "Example of an architecture targeted by Coyote")


## Cloning ##

Clone this repository including its submodules by executing

```git clone --recurse-submodules <repository>```

## Documentation ##

The documentation is available in the `Coyote/docs` directory. Execute `make` to compile the documentation into HTML format and open `Coyote/docs/html/index.html` in your browser. You will need [doxygen](https://www.doxygen.nl/ "Doxygen Homepage") to compile the documents.
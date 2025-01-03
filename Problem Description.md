### Objective
+ In this homework, our goal is to construct a matrix multiplication machine which utilizes multiprogramming to speed up computation.
+ The machine has to handle the following two tasks:
  + Each request contains two matrices. Multiply them.
  + Handle multiple requests simultaneously.
  
### High-Level Explanation
+ Task 1: Multiplying the Two Given Matrices
Each request contains two matrices that are guaranteed to be compatible for multiplication. Perform matrix multiplication on the given matrices (first matrix $\times$ second matrix).
+ Task 2: Handle Multiple Requests Simultaneously
In some test cases, there will be more than one request. You must handle multiple requests simultaneously. That is, you should not wait the calculation of current request finish before starting handling the next request.
In order to achieve this, you can divide each multiplication into many pieces of works and assign them to other threads.
You should be careful when dealing with it, as race conditions may occur since the works are globally shared. You can use any method taught in this course to protect the shared resources, i.e., critical section, but we recommend you to use mutex since using signal may be more complicated.

### Scope
+ Complete the data structures' setting in tpool.h.
+ Handle initializing the thread pools, receiving requests, synchronization, destruction in tpool.c.

### Environment Specification
+ OS: x86_64 Linux 6.6
+ C standard: -std=gnu23

### File Structure
+ main.c: The main program well-implemented by the TAs.
+ tpool.c: Implementations for multithreaded functions
+ tpool.h: Refine structures for threads

#### Compilation and Execution
+ Use make to build the project. Use make clean to clean all generated files. -std=gnu23 is required for main.c.
+ After built, there will be an executable called hw4 in the same directory, and there will a directory build/ storing intermediate files during the building process.
+ Use make all to force a clean build, i.e., remove all intermediate files before building.
+ The main program will consume testcase from stdin, and output the result to stdout. Therefore, to local test your code, run:
./hw4 < {inputfile} > {outputfile}
where {inputfile} is the path to the input, and {outputfile} is the path to the output.

#  6.827 Final Project: Increasing Compressed Graph Locality with Work-Efficient Row-Reordering

Builds on the work on serial affinity-based row-reordering done in GAMMA (Zhang, et. al. 2021)
[Link to paper](https://people.csail.mit.edu/emer/papers/2021.04.asplos.gamma.pdf)

This repo furnishes a set of tools for converting graphs to compressed-sparse row (CSR) format and for running row-reordering on graphs in CSR representation.

The serial affinity-based row-reordering algorithm is designed to increase the effiency of sparse matrix mulitplication (spGEMM, Z = A*B) by reordering the rows of A to maximize the reuse of rows of B.

The serial affinity-based row-reordering algorithm was designed to work on matrices in CSR format. The goal of this work is to design a theoretically work-efficient single-machine parallel implementation of affinity-based row-reordering which achieves a speedup against real and synthetic workloads.


## Build and run

```
make all
./sre < mat.csr.example
0
```


The `sre` tool runs **s**erial row-**re**ordering on the example CSR matrix provided in this repo. At time of writing the tool outputs the runtime in milliseconds of the row-reordering algorithm. This is to facilitate benchmarking.


Other tools in this repo
* `sut` - **s**erial implementations of **ut**ility functions. To convert a matrix from SparseSuit adjacency list format to CSR format, use `./sut < /path/to/mtx`
* `rcsr` - *r*andom **CSR** synthetic workload generator. The purpose of `rcsr` is to faclitate sweep tests of run-time for the row-reordering algorithm, with respect to key workload parameters. To generate a random .csr square matrix file, use `./rcsr <# rows> <density percent>`, i.e. `./rcsr 1000 5` for a 1000x1000 10% dense square matrix

## Sweep tests

This repo includes a Jupyter notebook, `Serial row-reordering experiments.ipynb`, which can be used to automate sweep tests of row-reordering run-time over matrix size and density.


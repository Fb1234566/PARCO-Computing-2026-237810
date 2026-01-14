#include "spvm_mpi.h"

#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);                     // Initialize MPI

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);// number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);// this process' rank

    printf("Hello from rank %d of %d\n", world_rank, world_size);

    // Example: broadcast a value from root (rank 0) to all processes
    int value;
    if (world_rank == 0) value = 12345;        // only root sets the value
    MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
    printf("Rank %d received broadcast value %d\n", world_rank, value);

    // Example: reduce (sum) values from all ranks to the root
    int my_val = world_rank;
    int sum = 0;
    MPI_Reduce(&my_val, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (world_rank == 0) {
        printf("Sum of ranks = %d\n", sum);
    }

    MPI_Finalize();                             // Clean up MPI
    return 0;
}
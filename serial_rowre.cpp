#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Asymmetric compressed-sparse row (CSR) representation
// *edges and *values must be allocated to length metadata_edges
// *vertices must be allocated to length metadata_rows + 1
//
// Key invariants:
// - Each edge is a destination node from the adjacency list
// - edges maintains the sorted order of the adjacency list
// - vertices is sorted (i.e. index corresponds to vertex id)
// - The egress-degree of vertex v is vertices[v+1] - vertices[v]
// - The last element of vertices is a dummy equal to metadata_edges,
//   to ensure that the last vertex's degree can be calculated
//
int metadata_rows = 0;
int metadata_columns = 0;
int metadata_edges = 0;
int *vertices, *edges;
double *values;

// A permutation of the rows of the CSR representation.
// *permutation must be allocated to length metadata_rows
int *permutation;

/*

Load a .csr asymmetric CSR representation from stdin

Format:
* Metadata line: rows columns edges
* VERTICES
* List of source vertices each followed by \n
* EDGES
* List of edge destination vertices each followed by \n
* VALUES
* List of edge values each followed by \n

*/
void load_mtx_csr_from_stdin() {

	char *serialized_data;
	size_t len;

	printf("Loading CSR matrix...\n");
	printf("- Loading metadata line and extracting.\n");
	assert(getline(&serialized_data, &len, stdin) != EOF);
	printf("%s\n",serialized_data);

	// Extract metadata
	sscanf(serialized_data, "%d %d %d", &metadata_rows, &metadata_columns, &metadata_edges);

	printf("- Rows: %d Columns: %d Edges: %d\n", metadata_rows, metadata_columns, metadata_edges);

	printf("- Allocating CSR representation memory...\n");

	// Allocating CSR memory
	edges = (int *) malloc(metadata_edges * sizeof(int));
	values = (double *) malloc(metadata_edges * sizeof(double));
	vertices = (int  *) malloc((metadata_rows + 1) * sizeof(int));

	printf("- Reading vertices...\n");

	// Read VERTICES preamble
	assert(getline(&serialized_data, &len, stdin) != EOF);
	assert(strcmp(serialized_data,"VERTICES\n") == 0);

	// Read vertices until we hit EDGES preamble
	int i=0;
	while (getline(&serialized_data, &len, stdin) != EOF && strcmp(serialized_data,"EDGES\n") != 0) {
		assert(i < (metadata_rows+1));
		int vertex_edge_edx = 0;
		sscanf(serialized_data, "%d", &vertex_edge_edx);
		vertices[i] = vertex_edge_edx;
		i++;
	}

	// Read edges until we hit VALUES preamble
	i=0;
	while (getline(&serialized_data, &len, stdin) != EOF && strcmp(serialized_data,"VALUES\n") != 0) {
		assert(i < metadata_edges);
		int edge_destination_vertex = 0;
		sscanf(serialized_data, "%d", &edge_destination_vertex);
		edges[i] = edge_destination_vertex;
		i++;
	}

	// Read values until EOF
	i=0;
	while (getline(&serialized_data, &len, stdin) != EOF) {
		assert(i < metadata_edges);
		double value = 0;
		sscanf(serialized_data, "%lf", &value);
		values[i] = value;
		i++;
	}
}

/*
Print head/tail of CSR representation
*/
void print_csr() {

        printf("- CSR preview:\n");

	printf("-- Vertices:\n");

        // Print vertices head

        for (int i=0; i < ((5 < (metadata_rows+1)) ? 5 : (metadata_rows+1)); i++) {

                printf("vertices[%d] == %d \n", i, vertices[i]);

        }

        printf("\n...\n\n");

        if (5 < metadata_rows) {

                // Print tail
                for (int i=((5 > (metadata_rows + 1) - 5) ? 5 : ((metadata_rows + 1) - 5)); i < metadata_rows + 1; i++) {

                        printf("vertices[%d] == %d \n", i, vertices[i]);

                }

        }

	printf("-- Edges and values:\n");

	// Print edges & values head

        for (int i=0; i < ((5 < metadata_edges) ? 5 : metadata_edges); i++) {

		printf("edges[%d] == %d values[%d] == %lf\n", i, edges[i], i, values[i]);

        }

        printf("\n...\n\n");

        if (5 < metadata_edges) {

                // Print tail
                for (int i=((5 > metadata_edges - 5) ? 5 : (metadata_edges - 5)); i < metadata_edges; i++) {

			printf("edges[%d] == %d values[%d] == %lf\n", i, edges[i], i, values[i]);

                }

        }

}

void serial_row_reorder() {
	permutation = (int  *) malloc((metadata_rows) * sizeof(int));
}

void print_permutation() {
	printf("Printing row permuation.\n\n");
	for (int i=0; i<metadata_rows)  printf("%d ", permutation[i]);
	printf("\n");
}

void free_all() {
	free(vertices);
	free(edges);
	free(values);
	free(permutation);
}

int main() {
	load_mtx_csr_from_stdin();
	print_csr();
	serial_row_reorder();
	print_permutation();
	free_all();
}

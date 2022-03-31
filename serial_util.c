#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define NO_INDEX -1 // Initial index value when converting adjacency list format to CSR

// Graph edge structure 
// User must enforce uni- or bi-directionality
typedef struct graph_edge {
	int source;
	int destination;
	double value;
} graph_edge;

// Sorted asymmetric adjacency list representation
// *edges must be allocated to length metadata_edges
int metadata_rows = 0;
int metadata_columns = 0;
int metadata_edges = 0;
graph_edge *edges;

/* 

Load a .mtx sorted asymmetric adjacency list from stdin 

Format:
* Comment preamble
* Metadata line
* One edge per line
* EOF

*/
void load_mtx_sorted_asymmetric_adjacency_list_from_stdin() {
	char *serialized_data;
	size_t len;

	printf("Skipping comment preamble...\n");

	// Scan through .mtx comment preamble until serialized_data holds the metadata line
	while (getline(&serialized_data, &len, stdin) != EOF && serialized_data[0] == '%');

	printf("Extracting metadata...\n");

	printf("%s\n",serialized_data);

	// Extract metadata
	sscanf(serialized_data, "%d %d %d", &metadata_rows, &metadata_columns, &metadata_edges);

	printf("Rows: %d Columns: %d Edges: %d\n", metadata_rows, metadata_columns, metadata_edges);

	printf("Allocating adjacency list memory...\n");

	// Allocate adjacency list memory
	edges = (graph_edge *) malloc(metadata_edges * sizeof(graph_edge));
	
	printf("Loading edges, printing head...\n");

	// Load edges 
	for (int i=0; i<metadata_edges; i++) {

                assert(getline(&serialized_data, &len, stdin) != EOF);

		sscanf(serialized_data, "%d %d %lf", &edges[i].source, &edges[i].destination, &edges[i].value);

		if (i < 10) {
			printf("edges[%d] == %d %d %lf\n", i, edges[i].source, edges[i].destination, edges[i].value);
		}

	}

	printf("Done.\n");
}

int main(void) {

	load_mtx_sorted_asymmetric_adjacency_list_from_stdin();

	return 0;
}

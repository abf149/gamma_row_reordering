#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define NO_INDEX -1 // Initial index value when converting adjacency list format to CSR

// Adjacency list graph edge structure 
// User must enforce uni- or bi-directionality
typedef struct adjacency_edge {
	int source;
	int destination;
	double value;
} adjacency_edge;

// Asymmetric adjacency list representation
// *edges must be allocated to length metadata_edges
int metadata_rows = 0;
int metadata_columns = 0;
int metadata_edges = 0;
adjacency_edge *adjacency_list;

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
int *vertices, *edges;
double *values;

/* 

Load a .mtx unsorted asymmetric adjacency list from stdin 

Format:
* Comment preamble
* Metadata line
* One edge per line
* EOF

*/
void load_mtx_unsorted_asymmetric_adjacency_list_from_stdin() {
	char *serialized_data;
	size_t len;

	printf("Loading adjacency list...");

	printf("- Skipping comment preamble...\n");

	// Scan through .mtx comment preamble until serialized_data holds the metadata line
	while (getline(&serialized_data, &len, stdin) != EOF && serialized_data[0] == '%');

	printf("- Extracting metadata...\n");

	printf("%s\n",serialized_data);

	// Extract metadata
	sscanf(serialized_data, "%d %d %d", &metadata_rows, &metadata_columns, &metadata_edges);

	printf("- Rows: %d Columns: %d Edges: %d\n", metadata_rows, metadata_columns, metadata_edges);

	printf("- Allocating adjacency list memory...\n");

	// Allocate adjacency list memory
	adjacency_list = (adjacency_edge *) malloc(metadata_edges * sizeof(adjacency_edge));
	
	printf("- Loading edges...\n");

	// Load edges 
	for (int i=0; i<metadata_edges; i++) {

                assert(getline(&serialized_data, &len, stdin) != EOF);

		sscanf(serialized_data, "%d %d %lf", &adjacency_list[i].source, &adjacency_list[i].destination, &adjacency_list[i].value);

		// Convert 1-indexing of vertices to 0-indexing
		adjacency_list[i].source -= 1;
		adjacency_list[i].destination -= 1;
	}

	printf("Done.\n");
}

/*

Compare two graph edges by their source

*/
int cmpfunc (const void * a, const void * b) {
	return ( (*(adjacency_edge *)a).source - (*(adjacency_edge *)b).source );
}

/*

Sort adjacency list by edge source

*/
void qsort_adjacency_list() {

	printf("Sorting adjacency list.\n");

	qsort(adjacency_list, metadata_edges, sizeof(adjacency_edge), cmpfunc);
}

/*

Convert loaded adjacency list to CSR format

*/
void convert_adjacency_list_to_csr() {

	printf("Converting adjacency list to CSR format.\n");

	printf("- Allocating CSR memory.\n");

	edges = (int *) malloc(metadata_edges * sizeof(int));
	values = (double *) malloc(metadata_edges * sizeof(double));
	vertices = (int  *) malloc((metadata_rows + 1) * sizeof(int));

	printf("- Converting adjacency list to CSR.\n");

	int prev_edge_edx = 0, source_last_seen = -1;

	for (int edx=0; edx < metadata_edges; edx++) {

		edges[edx] = adjacency_list[edx].destination;
		values[edx] = adjacency_list[edx].value;

		if (adjacency_list[edx].source != source_last_seen) {

			for (int vdx=source_last_seen + 1; vdx < adjacency_list[edx].source + 1; vdx++) {
				vertices[vdx] = edx;
			}

			source_last_seen = adjacency_list[edx].source;
		}

	}

	vertices[metadata_rows] = metadata_edges;

}

/*

Print head/tail of loaded, sorted adjacency list

*/
void print_adjacency_list() {

	printf("- Sorted adjacency list preview:\n\n");

	// Print head
	for (int i=0; i < ((10 < metadata_edges) ? 10 : metadata_edges); i++) {

		printf("edges[%d] == %d %d %lf\n", i, adjacency_list[i].source, adjacency_list[i].destination, adjacency_list[i].value);

	}

	printf("\n...\n\n");

	if (10 < metadata_edges) {

        	// Print tail
        	for (int i=((10 > metadata_edges - 10) ? 10 : (metadata_edges - 10)); i < metadata_edges; i++) {

                	printf("edges[%d] == %d %d %lf\n", i, adjacency_list[i].source, adjacency_list[i].destination, adjacency_list[i].value);

	        }

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

/*

Save CSR representation to file.

*/

void save_csr() {
	FILE * fp;

	fp = fopen("mat.csr", "w");

	printf("Saving CSR representation to file.\n");
	printf("- Writing metadata line.\n");
	fprintf(fp, "%d %d %d\n", metadata_rows, metadata_columns, metadata_edges);
	printf("- Writing vertices.\n");
	fprintf(fp, "VERTICES\n");
	for (int i=0; i < metadata_rows + 1; i++) fprintf(fp, "%d\n", vertices[i]);
	printf("- Writing edges.\n");
	fprintf(fp, "EDGES\n");
	for (int i=0; i < metadata_edges; i++) fprintf(fp, "%d\n", edges[i]);
	printf("- Writing values.\n");
	fprintf(fp, "VALUES\n");
	for (int i=0; i < metadata_edges; i++) fprintf(fp, "%lf\n", values[i]);

	fclose(fp);

}

int main(void) {

	load_mtx_unsorted_asymmetric_adjacency_list_from_stdin();

	qsort_adjacency_list();

	print_adjacency_list();

	convert_adjacency_list_to_csr();

	print_csr();

	save_csr();

	return 0;
}

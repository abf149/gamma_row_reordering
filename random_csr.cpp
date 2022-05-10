#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <assert.h>
#include <string.h>
#include <time.h>

using namespace std;

// Asymmetric adjacency list representation
// *edges must be allocated to length metadata_edges
int metadata_rows = 0;
int metadata_columns = 0;
int metadata_edges = 0;
double density_pct = 0.0;

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

Create a random square CSR matrix

*/

void random_square_csr() {

        printf("- Allocating CSR memory.\n");
        int* edges_tmp = (int *) malloc(metadata_edges*2 * sizeof(int));
        values = (double *) malloc(metadata_edges * sizeof(double));
        vertices = (int  *) malloc((metadata_rows + 1) * sizeof(int));

	 /* initialize random seed: */
  	srand (time(NULL));

	int edge_ptr = -1;

	for (int rdx=0; rdx<metadata_rows; rdx++) {
		vertices[rdx] = edge_ptr+1;
		for (int cdx=0; cdx<metadata_columns; cdx++) {
			if ( ((100.0 * rand() / (RAND_MAX + 1.0))) < density_pct) {
				edge_ptr++;
				edges_tmp[edge_ptr] = cdx;
				values[edge_ptr] = 1.0;
			}
		}
	}

	metadata_edges = edge_ptr+1;

	edges = (int *)malloc(metadata_edges * sizeof(int));
	memcpy(edges, edges_tmp, metadata_edges*sizeof(int));
	free(edges_tmp);

	vertices[metadata_rows] = metadata_edges;
}

/*

Print head/tail of CSR representation

*/
void print_csr() {

        printf("- CSR preview (%d rows, %d columns, %d edges, density %f%%):\n", metadata_rows, metadata_columns, metadata_edges, density_pct);

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

//	char fn[100];

//	sprintf(fn, "mat_n%d_d%d.csr");

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

void free_all() {
        free(vertices);
        free(edges);
        free(values);
}

int main(int argc, char *argv[]) {

	assert(argc == 4);

	metadata_rows = atoi(argv[1]);
	metadata_columns = atoi(argv[2]);

	metadata_edges = ((float)(metadata_rows*metadata_columns))*((float)((100.0 * rand() / (RAND_MAX + 1.0)) + 1));

	char *eptr;

	density_pct = strtod(argv[3],&eptr);

	printf("Density: %f\n",density_pct);

	random_square_csr();
	print_csr();
        save_csr();
        //free_all();

        return 0;
}


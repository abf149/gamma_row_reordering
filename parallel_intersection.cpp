#include <iostream>
//#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <chrono>
#include <cilk/cilk.h>
#include <cilk/reducer_opadd.h>

#define HEAP_ROOT 0
#define LEFT_CHILD(x) 2*x+1
#define RIGHT_CHILD(x) 2*x+2
#define PARENT(x) (x+1)/2-1
#define IS_LEFT_CHILD(x) (x+1)%2==0
#define IS_RIGHT_CHILD(x) (x+1)%2>0

using namespace std;
using chrono::high_resolution_clock;
using chrono::duration_cast;
using chrono::duration;
using chrono::milliseconds;

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

	string serialized_data;

	cout<<"Loading CSR matrix..."<<endl;
	cout<<"- Loading metadata line.\n"<<endl;
	getline(cin,serialized_data);
	cout<<serialized_data<<endl;

	// Extract metadata
	cout<<"- Extracting."<<endl;
	sscanf(serialized_data.c_str(), "%d %d %d", &metadata_rows, &metadata_columns, &metadata_edges);

	cout<<"- Rows: "<<metadata_rows<<" Columns: "<<metadata_columns<<" Edges: "<<metadata_edges<<endl;

	cout<<"- Allocating CSR representation memory..."<<endl;

	// Allocating CSR memory
	edges = (int *) malloc(metadata_edges * sizeof(int));
	values = (double *) malloc(metadata_edges * sizeof(double));
	vertices = (int  *) malloc((metadata_rows + 1) * sizeof(int));

	cout<<"- Reading vertices..."<<endl;

	// Read VERTICES preamble
	getline(cin,serialized_data);
        assert(strcmp(serialized_data.c_str(),"VERTICES") == 0);

	// Read vertices until we hit EDGES preamble
	int i=0;
	//cout<<"metadata_rows: "<<metadata_rows<<endl;
	while (getline(cin,serialized_data) && strcmp(serialized_data.c_str(),"EDGES") != 0) {
 		assert(i < (metadata_rows+1));
		int vertex_edge_edx = 0;
		sscanf(serialized_data.c_str(), "%d", &vertex_edge_edx);
		vertices[i] = vertex_edge_edx;
		i++;
	}

	// Read edges until we hit VALUES preamble
	i=0;
	while (getline(cin,serialized_data) && strcmp(serialized_data.c_str(),"VALUES") != 0) {
		assert(i < metadata_edges);
		int edge_destination_vertex = 0;
		sscanf(serialized_data.c_str(), "%d", &edge_destination_vertex);
		edges[i] = edge_destination_vertex;
		i++;
	}

	// Read values until EOF
	i=0;
	while (getline(cin,serialized_data)) {
		assert(i < metadata_edges);
		double value = 0;
		sscanf(serialized_data.c_str(), "%lf", &value);
		values[i] = value;
		i++;
	}
}

/*
Print head/tail of CSR representation
*/
void print_csr() {

        cout<<"- CSR preview:"<<endl;

	cout<<"-- Vertices:"<<endl;

        // Print vertices head

        for (int i=0; i < ((5 < (metadata_rows+1)) ? 5 : (metadata_rows+1)); i++) {

                cout<<"vertices["<<i<<"] == "<<vertices[i]<<endl;

        }

        cout<<endl<<"..."<<endl<<endl;

        if (5 < metadata_rows) {

                // Print tail
                for (int i=((5 > (metadata_rows + 1) - 5) ? 5 : ((metadata_rows + 1) - 5)); i < metadata_rows + 1; i++) {

                        cout<<"vertices["<<i<<"] == "<<vertices[i]<<endl;

                }

        }

	cout<<"-- Edges and values:"<<endl;

	// Print edges & values head

        for (int i=0; i < ((5 < metadata_edges) ? 5 : metadata_edges); i++) {

		cout<<"edges["<<i<<"] == "<<edges[i]<<" values["<<i<<"] == "<<values[i]<<endl;

        }

        cout<<endl<<"..."<<endl<<endl;

        if (5 < metadata_edges) {

                // Print tail
                for (int i=((5 > metadata_edges - 5) ? 5 : (metadata_edges - 5)); i < metadata_edges; i++) {

			cout<<"edges["<<i<<"] == "<<edges[i]<<" values["<<i<<"] == "<<values[i]<<endl;

                }

        }

}

// Affinity queue implementation

typedef struct pq_item {
	int row;
	int affinity;
} pq_item;

bool greater_pq(pq_item* lhs, pq_item* rhs) {
	return lhs->affinity > rhs->affinity;
}

void overwrite_pq_position(int dest, int source, vector<pq_item>* pq, vector<int>* row_positions) {
	vector<pq_item>& pqRef = *pq;
	vector<int>& row_positionsRef = *row_positions;
	pqRef[dest] = pqRef[source];
	row_positionsRef[pqRef[dest].row] = dest;
}

void swap_pq_positions(int i, int j, vector<pq_item>* pq, vector<int>* row_positions) {
        vector<pq_item>& pqRef = *pq;
        vector<int>& row_positionsRef = *row_positions;

	pq_item temp = pqRef[i];
	pqRef[i] = pqRef[j];
	pqRef[j] = temp;
	row_positionsRef[pqRef[i].row] = i;
	row_positionsRef[pqRef[j].row] = j;
}

pq_item pop_row(vector<pq_item>* pq, vector<int>* row_positions) {
        vector<pq_item>& pqRef = *pq;
        vector<int>& row_positionsRef = *row_positions;

	pq_item result = pqRef[HEAP_ROOT];

	// Remove max-affinity element
	int heap_size = pqRef.size();
	//cout<<"Popping: %d\n", pqRef[HEAP_ROOT].row);
	row_positionsRef[pqRef[HEAP_ROOT].row] = -1;
	overwrite_pq_position(HEAP_ROOT,heap_size-1,pq,row_positions);
	heap_size--;
	pqRef.pop_back();


	int i=HEAP_ROOT;
	bool brk=false;
	while (LEFT_CHILD(i) < heap_size && (!brk)) {
		// Compare to left child if right child is out-of-bounds or smaller
		if (RIGHT_CHILD(i) >= heap_size || pqRef[LEFT_CHILD(i)].affinity > pqRef[RIGHT_CHILD(i)].affinity) {
			if (pqRef[i].affinity < pqRef[LEFT_CHILD(i)].affinity) {
				swap_pq_positions(i, LEFT_CHILD(i), pq, row_positions);
				i = LEFT_CHILD(i);
			} else brk=true;
		} else {
			// Compare to right child
			if (pqRef[i].affinity < pqRef[RIGHT_CHILD(i)].affinity) {
				swap_pq_positions(i, RIGHT_CHILD(i), pq, row_positions);
				i = RIGHT_CHILD(i);
			} else brk=true;
		}
	}
	

	return result;
}

void increment_row_affinity(int row, vector<pq_item>* pq, vector<int>* row_positions) {
	//cout<<"****Incrementing affinity\n");

        vector<pq_item>& pqRef = *pq;
        vector<int>& row_positionsRef = *row_positions;

        // Increment row affinity
        int heap_size = pqRef.size();
	int i = row_positionsRef[row];
	pqRef[i].affinity++;

	// Reposition in heap
	bool brk=false;
	while (i > HEAP_ROOT && (!brk)) {
		if (pqRef[i].affinity > pqRef[PARENT(i)].affinity) {
			swap_pq_positions(i, PARENT(i), pq, row_positions);
			i = PARENT(i);
		} else brk=true;
	}
}

void decrement_row_affinity(int row, vector<pq_item>* pq, vector<int>* row_positions) {
        vector<pq_item>& pqRef = *pq;
        vector<int>& row_positionsRef = *row_positions;

        //cout<<"****Decrementing affinity\n");

        // Decrement row affinity
        int heap_size = pqRef.size();
        int i = row_positionsRef[row];
        pqRef[i].affinity--;

	// Reposition in heap
        bool brk = false;
        while (LEFT_CHILD(i) < heap_size && (!brk)) {
                // Compare to left child if right child is out-of-bounds or smaller
                if (RIGHT_CHILD(i) >= heap_size || pqRef[LEFT_CHILD(i)].affinity > pqRef[RIGHT_CHILD(i)].affinity) {
                        if (pqRef[i].affinity < pqRef[LEFT_CHILD(i)].affinity) {
                                swap_pq_positions(i, LEFT_CHILD(i), pq, row_positions);
                                i = LEFT_CHILD(i);
                        } else brk=true;
                } else {
                        // Compare to right child
                        if (pqRef[i].affinity < pqRef[RIGHT_CHILD(i)].affinity) {
                                swap_pq_positions(i, RIGHT_CHILD(i), pq, row_positions);
                                i = RIGHT_CHILD(i);
                        } else brk=true;
                }
        }
}

void print_priority_queue(vector<pq_item>* pq, vector<int>* row_positions) {

        vector<pq_item>& pqRef = *pq;
        vector<int>& row_positionsRef = *row_positions;

//	cout<<"- Priority queue state:\n");
//	cout<<"-- pq: [ ");
	for (int i=0; i<pq->size(); i++) cout<<"R"<<pqRef[i].row<<"A"<<pqRef[i].affinity<<" ";
//	cout<<"]\n");
//	cout<<"-- row_positions: [ ");
	for (int i=0; i<row_positions->size(); i++) cout<<row_positionsRef[i]<<" ";
//	cout<<"]\n");
}

void serial_row_reorder()
{
	auto t1 = high_resolution_clock::now();

	vector<pq_item> pq; // Priority-queue for row affinities
	vector<int> row_positions(metadata_rows, 0); // Positions of row affinities in Q
	permutation = (int  *) malloc(metadata_rows * sizeof(int));

        // Seed the permutation with the first row
        permutation[0] = 0;
	row_positions[0] = -1;
	for (int i=1; i<metadata_rows; i++) {
		// Add all rows to priority queue except the first
		pq_item temp;
		temp.row=i;
		temp.affinity=0;
		pq.push_back(temp);
		row_positions[i] = i-1;
	}

	// Algorithm tuning parameter
	int window = 10;

	// Using Fibertree notation
	int payload_length0=0, payload_length1=0; 
	int r0_coord=0, r1_coord=0;
	int edge_offset0=0, edge_offset1=0;
	int c0_pos=0, c1_pos=0, c0_coord=0;

	pq_item reordered_row;

//	print_priority_queue(&pq, &row_positions);

	// Greedily reorder one row at a time
	for (int r_permutation=1; r_permutation<metadata_rows; r_permutation++) {

//		cout<<"r_permutation == %d\n", r_permutation);

		// Examine the last reordered row, and
		r0_coord = permutation[r_permutation - 1];

//		cout<<"r0_coord == %d\n", r0_coord);

		// For each nonzero column position in compressed representation of the row,
		payload_length0 = vertices[r0_coord+1] - vertices[r0_coord];
		edge_offset0=vertices[r0_coord];
		for (c0_pos=0; c0_pos<payload_length0; c0_pos++) {
			c0_coord=edges[edge_offset0+c0_pos];
			//cout<<"c0_coord: %d\n", c0_coord);

			// For each un-reordered row, other than the one we just reordered,
			for (r1_coord=0; r1_coord<metadata_rows; r1_coord++) {
				if (r1_coord != r0_coord && (row_positions[r1_coord] > -1)) {

					// Step positionally through this compressed row and look for a nonzero
					// at the same position as in the row we just reordered
					payload_length1 = vertices[r1_coord+1] - vertices[r1_coord];
					edge_offset1=vertices[r1_coord];
					for (c1_pos=0; edges[edge_offset1+c1_pos] < c0_coord+1; c1_pos++) {
						//cout<<"edge_offset0: %d r0_coord: %d c0_coord: %d edge_offset1: %d r1_coord: %d c1_coord: %d\n", 
						//	edge_offset0,    r0_coord,    c0_coord,    edge_offset1,    r1_coord,    edges[edge_offset1+c1_pos]);
						if (c0_coord ==  edges[edge_offset1+c1_pos]) {
							//increase key
							increment_row_affinity(r1_coord, &pq, &row_positions);
							//print_priority_queue(&pq, &row_positions);
							payload_length1 = c1_pos; // break
						}
					}
				}
			}
		}

		if (r_permutation > window) {

	                // Examine the last reordered row, and
        	        r0_coord = permutation[r_permutation - window - 1];

	                // For each nonzero column position in compressed representation of the row,
        	        payload_length0 = vertices[r0_coord+1] - vertices[r0_coord];
                	edge_offset0=vertices[r0_coord];
	                for (c0_pos=0; c0_pos<payload_length0; c0_pos++) {
        	                c0_coord=edges[edge_offset0+c0_pos];

				//cout<<"DECR c0_coord: %d\n", c0_coord);

	                        // For each un-reordered row, other than the one we just reordered,
        	                for (r1_coord=0; r1_coord<metadata_rows; r1_coord++) {
                	                if (r1_coord != r0_coord && (row_positions[r1_coord] > -1)) {

        	                                // Step positionally through this compressed row and look for a nonzero
                	                        // at the same position as in the row we just reordered
                        	                payload_length1 = vertices[r1_coord+1] - vertices[r1_coord];
                                	        edge_offset1=vertices[r1_coord];
	                                        for (c1_pos=0; edges[edge_offset1+c1_pos] < c0_coord+1; c1_pos++) {
							//cout<<"DECR r0_coord: %d c0_coord: %d r1_coord: %d c1_coord: %d\n", r0_coord, c0_coord, r1_coord, edges[edge_offset1+c1_pos]);
        	                                        if (c0_coord ==  edges[edge_offset1+c1_pos]) {
								//decrease key
								decrement_row_affinity(r1_coord, &pq, &row_positions);
								//print_priority_queue(&pq, &row_positions);
        	                                                payload_length1 = c1_pos; // break
                	                                }
                        	                }
                                	}
	                        }
        	        }

		}

		reordered_row=pop_row(&pq, &row_positions);
//		cout<<"- reordered_row: %d affinity: %d\n", reordered_row.row, reordered_row.affinity);

		permutation[r_permutation] = reordered_row.row;
	}

        auto t2 = high_resolution_clock::now();

	auto ms_int = duration_cast<milliseconds>(t2-t1);

	cout<< ms_int.count() << endl;
}

void print_permutation() {
	cout<<"Printing row permuation."<<endl<<endl;
	for (int i=0; i<metadata_rows; i++)  cout<<permutation[i]<<" ";
	cout<<endl;
}

void free_all() {
	free(vertices);
	free(edges);
	free(values);
	free(permutation);
}

void parallel_row_intersection(){
        auto t1 = high_resolution_clock::now();

	int* intersection_vector = (int*)calloc(metadata_columns, sizeof(int));
	cilk::reducer_opadd<long long int> sum;

	long long int row_0_edge_count = vertices[1] - vertices[0];
	cout<<"row_0_edge_count: "<<row_0_edge_count<<endl;
	long long int row_1_edge_count = vertices[2] - vertices[1];
        cout<<"row_1_edge_count: "<<row_1_edge_count<<endl;
        long long int total_edge_combinations = row_0_edge_count*row_1_edge_count;
	cout<<"total_edge_combinations: "<<total_edge_combinations<<endl;
	cilk_for (long long int r=0; r<total_edge_combinations; r++) {
		long long int r0_pos = r % row_0_edge_count + ((long long int)vertices[0]);
		long long int r1_pos = ((long long int)r/row_0_edge_count) + ((long long int)vertices[1]);
		long long int r0_coord = edges[r0_pos];
		long long int r1_coord = edges[r1_pos];
		
		if (r0_coord == r1_coord) {
			sum++;
		}
	}
	cilk_sync;

/*
	cout<<"intersection_vector: [ ");
	for (int i=0; i<metadata_columns; i++) {
		cout<<"%d ", intersection_vector[i]);
	}
	cout<<"]\n");
*/

        auto t2 = high_resolution_clock::now();

        auto ms_int = duration_cast<milliseconds>(t2-t1);

        cout<<"Intersection runtime: "<< ms_int.count() << endl;

	cout<<"Sum: "<<sum.get_value()<<endl;
}

int main() {
	cout<<"Loading..."<<endl;
	load_mtx_csr_from_stdin();
	print_csr();
	cout<<"Intersecting rows..."<<endl;
	parallel_row_intersection();
//	print_permutation();
	cout<<"Freeing..."<<endl;
	free_all();
}

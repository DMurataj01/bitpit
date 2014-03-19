/*
 * Class_Para_Tree.hpp
 *
 *  Created on: 12/feb/2014
 *      Author: Marco Cisternino
 */

#ifndef CLASS_PARA_TREE_H_
#define CLASS_PARA_TREE_H_

// =================================================================================== //
// INCLUDES                                                                            //
// =================================================================================== //
#include "preprocessor_defines.dat"
#include "Class_Octant.hpp"
#include "Class_Local_Tree.hpp"
#include "Class_Comm_Buffer.hpp"
#include "map.hpp"
#include "Class_Array.hpp"
#include <cstdint>
#include <iterator>
#include <set>
#include "mpi.h"


// =================================================================================== //
// NAME SPACES                                                                         //
// =================================================================================== //
using namespace std;

// =================================================================================== //
// CLASS DEFINITION                                                                    //
// =================================================================================== //

class Class_Para_Tree {

	// ------------------------------------------------------------------------------- //
	// TYPEDEFS ----------------------------------------------------------------------- //
public:
	typedef vector<Class_Octant> 		OctantsType;
	typedef vector<uint32_t>			u32vector;
	typedef vector<vector<uint32_t>	>	u32vector2D;
	typedef vector<vector<uint64_t>	>	u64vector2D;
	typedef vector<vector<double>	>	dvector2D;

	// ------------------------------------------------------------------------------- //
	// MEMBERS ----------------------------------------------------------------------- //
public:
	//undistributed members
	uint64_t* partition_first_desc; 			//global array containing position of the first possible octant in each processor
	uint64_t* partition_last_desc; 				//global array containing position of the last possible octant in each processor
	uint64_t* partition_range_globalidx;	 	//global array containing global index of the last existing octant in each processor
	uint64_t global_num_octants;   				// global number of octants in the parallel octree
	map<int,vector<uint32_t> > bordersPerProc;	//local indices of border octants per process
	int nproc;
	uint8_t max_depth;							// global max existing level in the parallel octree

	//distributed members
	int rank;
	Class_Local_Tree octree;					// local tree in each processor

	//auxiliary members
	int error_flag;								// MPI error flag
	bool serial;								// 1 if the octree is the same on each processor, 0 if the octree is distributed

	//map member
	Class_Map trans;

	// connectivity
	dvector2D					nodes;				// Local vector of nodes (x,y,z) ordered with Morton Number
	u32vector2D					connectivity;		// Local vector of connectivity (node1, node2, ...) ordered with Morton-order.
													// The nodes are stored as index of vector nodes
	dvector2D					ghostsnodes;		// Local vector of ghosts nodes (x,y,z) ordered with Morton Number
	u32vector2D					ghostsconnectivity;	// Local vector of ghosts connectivity (node1, node2, ...) ordered with Morton-order.
													// The nodes are stored as index of vector nodes

	// ------------------------------------------------------------------------------- //
	// CONSTRUCTORS ------------------------------------------------------------------ //
public:
	Class_Para_Tree();
	Class_Para_Tree(double & X, double & Y, double & Z, double & L);
	~Class_Para_Tree();
	// ------------------------------------------------------------------------------- //
	// METHODS ----------------------------------------------------------------------- //
	void loadBalance();							//assign the octants to the processes following a computed partition
	bool adapt();  								//call refine and coarse on the local tree
	bool adapt(u32vector & mapidx);  			//call refine and coarse on the local tree
												// mapidx[i] = index in old octants vector of the i-th octant (index of father or first child if octant is new after refine or coarse)
	void updateAdapt();							//update Class_Para_Tree members after a refine and/or coarse
	void updateAfterCoarse();					//update Class_Para_Tree members and delete overlapping octants after a coarse
	void updateLoadBalance();					//update Class_Para_Tree members after a load balance
	void computePartition(uint32_t* partition); // compute octant partition giving the same number of octant to each process and redistributing the reminder
	int findOwner(const uint64_t & morton);		// given the morton of an octant it find the process owning that octant
	void setPboundGhosts(); 			 		// set pbound and build ghosts after static load balance
	void commMarker();							// communicates marker of ghosts
	void balance21();							// 2:1 balancing of parallel octree
	template<class UserDataComm>
	void communicate(UserDataComm & userData);
	
	void computeConnectivity();						// Computes nodes vector and connectivity of octants of local tree
	void clearConnectivity();						// Clear nodes vector and connectivity of octants of local tree
	void updateConnectivity();						// Updates nodes vector and connectivity of octants of local tree
	void computeghostsConnectivity();				// Computes ghosts nodes vector and connectivity of ghosts octants of local tree
	void clearghostsConnectivity();					// Clear ghosts nodes vector and connectivity of ghosts octants of local tree
	void updateghostsConnectivity();					// Update ghosts nodes vector and connectivity of ghosts octants of local tree

	// --------------------------------------------------------------------------------------------- //
	// Basic Get Methods --------------------------------------------------------------------------- //

public:
	double		getX(Class_Octant* const oct);
	double		getY(Class_Octant* const oct);
	double		getZ(Class_Octant* const oct);
	double		getSize(Class_Octant* const oct);		// Get the size of octant if mapped in hypercube
	double		getArea(Class_Octant* const oct);		// Get the face area of octant
	double		getVolume(Class_Octant* const oct);		// Get the volume of octant
	void		getCenter(Class_Octant* oct, 			// Get a vector of DIM with the coordinates of the center of octant
				vector<double> & center);
	void		getNodes(Class_Octant* oct, 			// Get a vector of vector (size [nnodes][3]) with the nodes of octant
				vector<vector<double> > & nodes);

};

#include "Class_Para_Tree.tpp"

#endif /* CLASS_PARA_TREE_H_ */

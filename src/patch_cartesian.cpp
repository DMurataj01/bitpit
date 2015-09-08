//
// Written by Andrea Iob <andrea_iob@hotmail.com>
//

/*!
	\class PatchCartesian

	\brief The PatchCartesian defines a Cartesian patch.

	PatchCartesian defines a Cartesian patch.
*/

#include "patch_cartesian.hpp"

#include <math.h>

namespace pman {

const int PatchCartesian::SPACE_MAX_DIM = 3;

/*!
	Creates a new patch.
*/
PatchCartesian::PatchCartesian(const int &id, const int &dimension,
					 std::array<double, 3> origin, double length, double dh)
	: Patch(id, dimension)
{
	std::cout << ">> Initializing cartesian mesh\n";

	// Info sulle celle
	m_cellSize.resize(dimension);
	m_minCoord.resize(dimension);
	m_nCells1D.resize(dimension);
	for (int n = 0; n < dimension; n++) {
		// Dimensioni della cella
		m_cellSize[n] = dh;

		// Numero di celle
		m_nCells1D[n] = (int) ceil(length / m_cellSize[n]);

		std::cout << "  - Cell count along direction " << n << " : " << m_nCells1D[n] << "\n";

		// Minima coordinata del dominio
		m_minCoord[n] = origin[n] - 0.5 * (m_nCells1D[n] * m_cellSize[n]);
	}

	m_cell_volume = m_cellSize[Node::COORD_X] * m_cellSize[Node::COORD_Y];
	if (is_three_dimensional()) {
		m_cell_volume *= m_cellSize[Node::COORD_Z];
	}

	// Info sui vertici
	m_nVertices1D.resize(dimension);
	for (int n = 0; n < dimension; n++) {
		if (!is_three_dimensional() && n == Node::COORD_Z) {
			m_nVertices1D[n] = 0;
		} else {
			m_nVertices1D[n] = m_nCells1D[n] + 1;
		}
	}

	// Info sulle interfacce
	m_x_nInterfaces1D.resize(dimension);
	for (int n = 0; n < dimension; n++) {
		m_x_nInterfaces1D[n] = m_nCells1D[n];
		if (n == Node::COORD_X) {
			m_x_nInterfaces1D[n]++;
		}
	}

	m_y_nInterfaces1D.resize(dimension);
	for (int n = 0; n < dimension; n++) {
		m_y_nInterfaces1D[n] = m_nCells1D[n];
		if (n == Node::COORD_Y) {
			m_y_nInterfaces1D[n]++;
		}
	}


	if (is_three_dimensional()) {
		m_z_nInterfaces1D.resize(dimension);
		for (int n = 0; n < dimension; n++) {
			m_z_nInterfaces1D[n] = m_nCells1D[n];
			if (n == Node::COORD_Z) {
				m_z_nInterfaces1D[n]++;
			}
		}
	}

	m_x_interface_area = m_cellSize[Node::COORD_Y];
	m_y_interface_area = m_cellSize[Node::COORD_X];
	if (is_three_dimensional()) {
		m_x_interface_area *= m_cellSize[Node::COORD_Z];
		m_y_interface_area *= m_cellSize[Node::COORD_Z];
		m_z_interface_area  = m_cellSize[Node::COORD_X] * m_cellSize[Node::COORD_Y];
	}

	m_normals = std::unique_ptr<CollapsedArray2D<double> >(new CollapsedArray2D<double>(2 * dimension, 2 * dimension * dimension));
	for (int n = -1; n <= 1; n += 2) {
		for (int i = 0; i < dimension; i++) {
			std::vector<double> normal(dimension, 0);
			normal[i] = n;

			m_normals->push_back(dimension, normal.data());
		}
	}
}

/*!
	Destroys the patch.
*/
PatchCartesian::~PatchCartesian()
{
}

/*!
	Gets a pointer to the the opposite normal.

	\param normal is a pointer to the normal
	\result A pointer to the opposite normal.
 */
double * PatchCartesian::_get_opposite_normal(double *normal)
{
	int dimension = get_dimension();

	int id_current  = std::distance(m_normals->data(), normal) / dimension;
	int id_opposite = (id_current + dimension) % (2 * dimension);

	return m_normals->get(id_opposite);
}

/*!
	Updates the patch.

	\result Returns true if the mesh was updated, false otherwise.
*/
bool PatchCartesian::_update(std::vector<uint32_t> &cellMapping)
{
	UNUSED(cellMapping);

	if (!is_dirty()) {
		return false;
	}

	std::cout << ">> Updating cartesian mesh\n";

	// Definition of the mesh
	create_vertices();
	create_cells();
	create_interfaces();

	return true;
}

/*!
	Creates the vertices of the patch.
*/
void PatchCartesian::create_vertices()
{
	std::cout << "  >> Creating vertices\n";

	// Reset
	reset_vertices();

	// Definition of the vertices
	m_x = std::vector<double>(m_nVertices1D[Node::COORD_X]);
	for (int i = 0; i < m_nVertices1D[Node::COORD_X]; i++) {
		m_x[i] = m_minCoord[Node::COORD_X] + i * m_cellSize[Node::COORD_X];
	}

	m_y = std::vector<double>(m_nVertices1D[Node::COORD_Y]);
	for (int j = 0; j < m_nVertices1D[Node::COORD_Y]; j++) {
		m_y[j] = m_minCoord[Node::COORD_Y] + j * m_cellSize[Node::COORD_Y];
	}

	if (is_three_dimensional()) {
		m_z = std::vector<double>(m_nVertices1D[Node::COORD_Z]);
		for (int k = 0; k < m_nVertices1D[Node::COORD_Z]; k++) {
			m_z[k] = m_minCoord[Node::COORD_Z] + k * m_cellSize[Node::COORD_Z];
		}
	} else {
		m_z = std::vector<double>(0);
	}

	int nTotalVertices = 1;
	for (int n = 0; n < get_dimension(); n++) {
		nTotalVertices *= m_nVertices1D[n];
	}

	std::cout << "    - Vertex count: " << nTotalVertices << "\n";

	m_vertices.reserve(nTotalVertices);

	for (int i = 0; i < m_nVertices1D[Node::COORD_X]; i++) {
		for (int j = 0; j < m_nVertices1D[Node::COORD_Y]; j++) {
			for (int k = 0; (is_three_dimensional()) ? (k < m_nVertices1D[Node::COORD_Z]) : (k <= 0); k++) {
				int id_vertex = vertex_ijk_to_id(i, j, k);
				m_vertices.emplace_back(id_vertex);
				Node &vertex = m_vertices.back();

				// Coordinate
				std::unique_ptr<double[]> coords = std::unique_ptr<double[]>(new double[get_dimension()]);
				coords[Node::COORD_X] = m_x[i];
				coords[Node::COORD_Y] = m_y[j];
				if (is_three_dimensional()) {
					coords[Node::COORD_Z] = m_z[k];
				}

				vertex.set_coords(std::move(coords));
			}
		}
	}
}

/*!
	Creates the cells of the patch.
*/
void PatchCartesian::create_cells()
{
	std::cout << "  >> Creating cells\n";

	// Reset
	reset_cells();

	// Info on the cells
	int nCellVertices;
	if (is_three_dimensional()) {
		nCellVertices = Element::get_vertex_count(Element::BRICK);
	} else {
		nCellVertices = Element::get_vertex_count(Element::RECTANGLE);
	}

	// Count the cells
	int nTotalCells = 1;
	for (int n = 0; n < get_dimension(); n++) {
		nTotalCells *= m_nCells1D[n];
	}

	std::cout << "    - Cell count: " << nTotalCells << "\n";

	m_cells.reserve(nTotalCells);

	// Create the cells
	double cellCenter[get_dimension()];
	for (int i = 0; i < m_nCells1D[Node::COORD_X]; i++) {
		cellCenter[Node::COORD_X] = 0.5 * (m_x[i] + m_x[i+1]);
		for (int j = 0; j < m_nCells1D[Node::COORD_Y]; j++) {
			cellCenter[Node::COORD_Y] = 0.5 * (m_y[j] + m_y[j+1]);
			for (int k = 0; (is_three_dimensional()) ? (k < m_nCells1D[Node::COORD_Z]) : (k <= 0); k++) {
				int id_cell = cell_ijk_to_id(i, j, k);
				m_cells.emplace_back(id_cell, this);
				Cell &cell = m_cells.back();

				// Tipo
				if (is_three_dimensional()) {
					cell.set_type(Element::BRICK);
				} else {
					cell.set_type(Element::RECTANGLE);
				}

				// Position type
				cell.set_position_type(Cell::INTERNAL);

				// Volume
				cell.set_volume(&m_cell_volume);

				// Centroide
				if (is_three_dimensional()) {
					cellCenter[Node::COORD_Z] = 0.5 * (m_z[k] + m_z[k+1]);
				}

				std::unique_ptr<double[]> centroid = std::unique_ptr<double[]>(new double[get_dimension()]);
				std::copy_n(cellCenter, get_dimension(), centroid.get());

				cell.set_centroid(std::move(centroid));

				// Connettività
				std::unique_ptr<int[]> connect = std::unique_ptr<int[]>(new int[nCellVertices]);
				connect[0] = vertex_ijk_to_id(i,     j,     k);
				connect[1] = vertex_ijk_to_id(i + 1, j,     k);
				connect[2] = vertex_ijk_to_id(i + 1, j + 1, k);
				connect[3] = vertex_ijk_to_id(i    , j + 1, k);
				if (is_three_dimensional()) {
					connect[4] = vertex_ijk_to_id(i,     j,     k + 1);
					connect[5] = vertex_ijk_to_id(i + 1, j,     k + 1);
					connect[6] = vertex_ijk_to_id(i + 1, j + 1, k + 1);
					connect[7] = vertex_ijk_to_id(i    , j + 1, k + 1);
				}

				cell.set_connect(std::move(connect));
			}
		}
	}
}

/*!
	Creates the interfaces of the patch.
*/
void PatchCartesian::create_interfaces()
{
	std::cout << "  >> Creating interfaces\n";

	// Reset
	reset_interfaces();

	// Count the interfaces
	int nTotalInterfaces = 0;
	nTotalInterfaces += count_interfaces_direction(Node::COORD_X);
	nTotalInterfaces += count_interfaces_direction(Node::COORD_Y);
	if (is_three_dimensional()) {
		nTotalInterfaces += count_interfaces_direction(Node::COORD_Z);
	}

	std::cout << "    - Interface count: " << nTotalInterfaces << "\n";

	m_interfaces.reserve(nTotalInterfaces);

	// Allocate the space for interface information on the cells
	int nCellFaces;
	if (is_three_dimensional()) {
		nCellFaces = Element::get_face_count(Element::BRICK);
	} else {
		nCellFaces = Element::get_face_count(Element::RECTANGLE);
	}

	int nInterfacesForSide[nCellFaces];
	std::fill_n(nInterfacesForSide, nCellFaces, 1);

	for (unsigned int n = 0; n < m_cells.size(); n++) {
		m_cells[n].initialize_empty_interfaces(nInterfacesForSide);
	}

	// Create the interfaces
	create_interfaces_direction(Node::COORD_X);
	create_interfaces_direction(Node::COORD_Y);
	if (is_three_dimensional()) {
		create_interfaces_direction(Node::COORD_Z);
	}
}

/*!
	Counts the interfaces normal to the given direction.

	\param direction the method will count the interfaces normal to this
	                 direction
*/
int PatchCartesian::count_interfaces_direction(const Node::Coordinate &direction)
{
	std::vector<int> *interfaceCount1D;
	switch (direction)  {

	case Node::COORD_X:
		interfaceCount1D = &m_x_nInterfaces1D;
		break;

	case Node::COORD_Y:
		interfaceCount1D = &m_y_nInterfaces1D;
		break;

	case Node::COORD_Z:
		interfaceCount1D = &m_z_nInterfaces1D;
		break;

	}

	int nInterfaces = 1;
	for (int n = 0; n < get_dimension(); n++) {
		nInterfaces *= (*interfaceCount1D)[n];
	}

	return nInterfaces;
}

/*!
	Creates the interfaces normal to the given direction.

	\param direction the method will creat the interfaces normal to this
	                 direction
*/
void PatchCartesian::create_interfaces_direction(const Node::Coordinate &direction)
{
	std::cout << "  >> Creating interfaces normal to direction " << direction << "\n";

	// Info on the interfaces
	int nInterfaceVertices;
	if (is_three_dimensional()) {
		nInterfaceVertices = Element::get_face_count(Element::RECTANGLE);
	} else {
		nInterfaceVertices = Element::get_face_count(Element::LINE);
	}

	double *area;
	std::vector<int> *interfaceCount1D;
	switch (direction)  {

	case Node::COORD_X:
		area = &m_x_interface_area;
		interfaceCount1D = &m_x_nInterfaces1D;
		break;

	case Node::COORD_Y:
		area = &m_y_interface_area;
		interfaceCount1D = &m_y_nInterfaces1D;
		break;

	case Node::COORD_Z:
		area = &m_z_interface_area;
		interfaceCount1D = &m_z_nInterfaces1D;
		break;

	}

	// Counters
	int counters[SPACE_MAX_DIM] = {0, 0, 0};
	int &i = counters[Node::COORD_X];
	int &j = counters[Node::COORD_Y];
	int &k = counters[Node::COORD_Z];

	// Creation of the interfaces
	for (i = 0; i < (*interfaceCount1D)[Node::COORD_X]; i++) {
		for (j = 0; j < (*interfaceCount1D)[Node::COORD_Y]; j++) {
			for (k = 0; (is_three_dimensional()) ? (k < (*interfaceCount1D)[Node::COORD_Z]) : (k <= 0); k++) {
				int id_interface = interface_nijk_to_id(direction, i, j, k);
				m_interfaces.emplace_back(id_interface, this);
				Interface &interface = m_interfaces.back();

				// Area
				interface.set_area(area);

				// Position
				Interface::PositionType position;
				if (counters[direction] == 0 || counters[direction] == (*interfaceCount1D)[direction] - 1) {
					position = Interface::BOUNDARY;
				} else {
					position = Interface::INTERNAL;
				}

				interface.set_position_type(position);

				// Owner
				int ownerIJK[SPACE_MAX_DIM];
				for (int n = 0; n < SPACE_MAX_DIM; n++) {
					ownerIJK[n] = counters[n];
				}
				if (counters[direction] > 0) {
					ownerIJK[direction] -= 1;
				}
				Cell &owner = m_cells[cell_ijk_to_id(ownerIJK)];

				int ownerFace = 2 * direction;
				if (counters[direction] == 0) {
					ownerFace++;
				}

				interface.set_owner(owner.get_id(), ownerFace);
				owner.set_interface(ownerFace, 0, interface.get_id());

				// Neighbour
				if (position != Interface::BOUNDARY) {
					int neighIJK[SPACE_MAX_DIM];
					for (int n = 0; n < SPACE_MAX_DIM; n++) {
						neighIJK[n] = counters[n];
					}

					Cell &neigh = m_cells[cell_ijk_to_id(neighIJK)];

					int neighFace = 2 * direction + 1;

					interface.set_neigh(neigh.get_id(), neighFace);
					neigh.set_interface(neighFace, 0, interface.get_id());
				} else {
					interface.unset_neigh();
				}

				// Connectivity
				std::unique_ptr<int[]> connect = std::unique_ptr<int[]>(new int[nInterfaceVertices]);
				if (direction == Node::COORD_X) {
					connect[0] = vertex_ijk_to_id(i, j,     k);
					connect[1] = vertex_ijk_to_id(i, j + 1, k);
					if (is_three_dimensional()) {
						connect[2] = vertex_ijk_to_id(i, j + 1, k + 1);
						connect[3] = vertex_ijk_to_id(i, j,     k + 1);
					}
				} else if (direction == Node::COORD_Y) {
					connect[0] = vertex_ijk_to_id(i,     j,     k);
					connect[1] = vertex_ijk_to_id(i + 1, j,     k);
					if (is_three_dimensional()) {
						connect[2] = vertex_ijk_to_id(i + 1, j, k + 1);
						connect[3] = vertex_ijk_to_id(i,     j, k + 1);
					}
				} else if (direction == Node::COORD_Z) {
					connect[0] = vertex_ijk_to_id(i,     j,     k);
					connect[1] = vertex_ijk_to_id(i + 1, j,     k);
					if (is_three_dimensional()) {
						connect[2] = vertex_ijk_to_id(i + 1, j + 1, k);
						connect[3] = vertex_ijk_to_id(i,     j + 1, k);
					}
				}

				interface.set_connect(std::move(connect));

				// Centroid
				std::unique_ptr<double[]> centroid = std::unique_ptr<double[]>(new double[get_dimension()]);
				std::fill_n(centroid.get(), get_dimension(), 0.0);

				for (int n = 0; n < nInterfaceVertices; n++) {
					Node &vertex = m_vertices[interface.get_vertex(n)];
					double *vertexCoords = vertex.get_coords();

					for (int k = 0; k < get_dimension(); k++) {
						centroid[k] += vertexCoords[k];
					}
				}

				for (int k = 0; k < get_dimension(); k++) {
					centroid[k] /= nInterfaceVertices;
				}

				interface.set_centroid(std::move(centroid));

				// Normal
				interface.set_normal(m_normals->get(ownerFace));
			}
		}
	}
}

/*!
	Marks a cell for refinement.

	This is a void function since mesh refinement is not implemented
	for Cartesian meshes.

	\param cell the cell to be refined
*/
void PatchCartesian::_mark_for_refinement(Cell &cell)
{
	UNUSED(cell);
}

/*!
	Converts the cell (i, j, k) notation to a single index notation
*/
int PatchCartesian::cell_ijk_to_id(const int &i, const int &j, const int &k) const
{
	int id = i;
	id += m_nCells1D[Node::COORD_X] * j;
	if (get_dimension() == 3) {
		id += m_nCells1D[Node::COORD_Y] * m_nCells1D[Node::COORD_X] * k;
	}

	return id;
}

/*!
	Converts the cell (i, j, k) notation to a single index notation
*/
int PatchCartesian::cell_ijk_to_id(const int ijk[]) const
{
	return cell_ijk_to_id(ijk[Node::COORD_X], ijk[Node::COORD_Y], ijk[Node::COORD_Z]);
}


/*!
	Converts the vertex (i, j, k) notation to a single index notation
*/
int PatchCartesian::vertex_ijk_to_id(const int &i, const int &j, const int &k) const
{
	int id = i;
	id += m_nVertices1D[Node::COORD_X] * j;
	if (get_dimension() == 3) {
		id += m_nVertices1D[Node::COORD_Y] * m_nVertices1D[Node::COORD_X] * k;
	}

	return id;
}

/*!
	Converts the vertex (i, j, k) notation to a single index notation
*/
int PatchCartesian::vertex_ijk_to_id(const int ijk[]) const
{
	return vertex_ijk_to_id(ijk[Node::COORD_X], ijk[Node::COORD_Y], ijk[Node::COORD_Z]);
}

/*!
	Converts the interface (normal, i, j, k) notation to a single index notation
*/
int PatchCartesian::interface_nijk_to_id(const int &normal, const int &i, const int &j, const int &k) const
{
	const int *nInterfaces;
	switch (normal) {

	case Node::COORD_X:
		nInterfaces = m_x_nInterfaces1D.data();
		break;

	case Node::COORD_Y:
		nInterfaces = m_y_nInterfaces1D.data();
		break;

	case Node::COORD_Z:
		nInterfaces = m_z_nInterfaces1D.data();
		break;

	}

	int id = i;
	id += nInterfaces[Node::COORD_X] * j;
	if (is_three_dimensional()) {
		id += nInterfaces[Node::COORD_Y] * nInterfaces[Node::COORD_X] * k;
		id += nInterfaces[Node::COORD_Z] * nInterfaces[Node::COORD_Y] * nInterfaces[Node::COORD_X] * normal;
	} else {
		id += nInterfaces[Node::COORD_Y] * nInterfaces[Node::COORD_X] * normal;
	}

	return id;
}

}

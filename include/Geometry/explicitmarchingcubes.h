#ifndef explicitmarchingcubes_h
#define explicitmarchingcubes_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2006
 RCS:           $Id: explicitmarchingcubes.h,v 1.4 2007-09-06 19:32:43 cvskris Exp $
________________________________________________________________________

-*/

#include "multidimstorage.h"
#include "thread.h"

class MarchingCubesSurface;
class ExplicitMarchingCubesSurfaceBucket;
template <class T> class SamplingData;
template <class T> class Interval;
class CoordList;

/*!A triangulated representation of an MarchingCubesSurface. */


class ExplicitMarchingCubesSurface
{
public:
			ExplicitMarchingCubesSurface(MarchingCubesSurface*);
			~ExplicitMarchingCubesSurface();

    void			setSurface(MarchingCubesSurface*);
    MarchingCubesSurface*	getSurface() { return surface_; }
    const MarchingCubesSurface*	getSurface() const { return surface_; }
    void			setCoordList(CoordList* cl, CoordList* nl=0 );
    void			setRightHandedNormals(bool yn=true);
    void			removeAll();

    bool			update();
    bool			update(const Interval<int>& xrg,
	    			       const Interval<int>& yrg,
	    			       const Interval<int>& zrg);

    void			setAxisScales(const SamplingData<float>& xrg,
					      const SamplingData<float>& yrg,
					      const SamplingData<float>& zrg);
    				/*!<If set, the coordinates in each dim will be
				    scaled with these scales. */
    const SamplingData<float>&	getAxisScale( int dim ) const;

    int				nrIndicesSets() const;
    				//!<Number is the same for both coords & normals
    int				nrCoordIndices(int set) const;
    				/*!<Check with needsUpdate() if indices
				    are up to date. */
    const int32*		getCoordIndices(int set) const;
				/*!<Check with needsUpdate() if indices are
				    up to date. */
    int				nrNormalIndices(int set) const;
    				 /*!<Check with needsUpdate() if indices
				     are up to date. Normals are generated
				     per face (triangle). */
    const int32*		getNormalIndices(int set) const;
				/*!<Check with needsUpdate() if indices are
				    up to date.Normals are generated
				    per face (triangle).*/

    bool			needsUpdate() const { return true; }

protected:
    friend		class ExplicitMarchingCubesSurfaceUpdater;

    bool		updateIndices(const int* pos);
    bool		getCoordIndices(const int* pos,int* res);
    bool		updateCoordinates(const int* pos);
    bool		updateCoordinate(const int* pos,int*res);

    MarchingCubesSurface*	surface_;

    SamplingData<float>*	scale0_;
    SamplingData<float>*	scale1_;
    SamplingData<float>*	scale2_;


    CoordList*			coordlist_;
    CoordList*			normallist_;
    bool			righthandednormals_;
    MultiDimStorage<int>	coordindices_;
    Threads::ReadWriteLock	coordindiceslock_;

    MultiDimStorage<ExplicitMarchingCubesSurfaceBucket*>	ibuckets_;
    ObjectSet<ExplicitMarchingCubesSurfaceBucket>		ibucketsset_;
    Threads::ReadWriteLock					ibucketslock_;
};

/*!Describes one or more triangle setups for a specific point configuration.
The indices are grouped in pairs, where a pair represents a coordinate.
The first number in the pair describes which neighbor that is responsible
for the point using the following coding:

   -1 - Triangle strip is ending. No second number will follow. Next number
        is first number on next point.
    0 - my own point
    1 - neighbor z+1
    2 - neighbor y+1
    3 - neighbor y+1, z+1
    4 - neighbor x+1
    5 - neighbor x+1 z+1
    6 - neighbor x+1 y+1
    7 - neighbor x+1 y+1, z+1

The (eventual) second number says which of the neighbor three coordinates is
wanted.
   0 = The coordinate on the x-axis
   1 = the coordinate on the y-axis
   2 = the coordinate on the z-axis.
*/

class MarchingCubeTriangleTable
{
public:
    ObjectSet<char>		indices_;
    TypeSet<unsigned char>	nrindices_;
    				//!Specifies how many pairs of indices that are
				//!in each list + the number of -1s.
};


/*!Lookup table with one MarchingCubeTriangleTable per position constellation.*/


class MarchingCubeLookupTable
{
public:
    				MarchingCubeLookupTable();
    				~MarchingCubeLookupTable();
    MarchingCubeTriangleTable	triangles_[256];

    static const MarchingCubeLookupTable& get();
};

#endif

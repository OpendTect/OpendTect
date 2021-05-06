#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2006
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "indexedshape.h"
#include "multidimstorage.h"
#include "thread.h"

class MarchingCubesSurface;
template <class T> class SamplingData;
class TaskRunner;

/*!A triangulated representation of an MarchingCubesSurface. */


mExpClass(Geometry) ExplicitMarchingCubesSurface : public CallBacker,
						   public Geometry::IndexedShape
{
public:
			ExplicitMarchingCubesSurface(MarchingCubesSurface*);
			~ExplicitMarchingCubesSurface();

    void			setSurface(MarchingCubesSurface*);
    MarchingCubesSurface*	getSurface() { return surface_; }
    const MarchingCubesSurface*	getSurface() const { return surface_; }
    void			removeAll(bool deep);

    bool			update(bool forceall,TaskRunner* = 0);
    bool			needsUpdate() const;

    bool			createsNormals() const { return true; }

protected:

    friend		class ExplicitMarchingCubesSurfaceUpdater;
    void		surfaceChange(CallBacker*);

    bool		allBucketsHaveChanged() const;

    bool		update(const Interval<int>& xrg,
	    		       const Interval<int>& yrg,
	    		       const Interval<int>& zrg,TaskRunner* = 0);
    void		removeBuckets(const Interval<int>& xrg,
	    			      const Interval<int>& yrg,
				      const Interval<int>& zrg);
    bool		updateIndices(const int* pos);
    bool		getCoordIndices(const int* pos,int* res);
    bool		updateCoordinates(const int* pos);
    bool		updateCoordinate(const int* pos,const int* idxs,
	    				 int*res);
    int			getBucketPos(int pos) const;

    MarchingCubesSurface*	surface_;

    Interval<int>*		changedbucketranges_[3];
    int				lastversionupdate_;

    MultiDimStorage<int>	coordindices_;
    Threads::Lock		coordindiceslock_;

    MultiDimStorage<Geometry::IndexedGeometry*>	ibuckets_;
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

mExpClass(Geometry) MarchingCubeTriangleTable
{
public:
    ObjectSet<char>		indices_;
    TypeSet<unsigned char>	nrindices_;
    				//!Specifies how many pairs of indices that are
				//!in each list + the number of -1s.
};


/*!Lookup table with one MarchingCubeTriangleTable per position constellation.*/


mExpClass(Geometry) MarchingCubeLookupTable
{
public:
    				MarchingCubeLookupTable();
    				~MarchingCubeLookupTable();
    MarchingCubeTriangleTable	triangles_[256];

    static const MarchingCubeLookupTable& get();
};


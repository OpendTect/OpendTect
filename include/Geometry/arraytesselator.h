#pragma once
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          April 2011
RCS:           $Id$
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "arraynd.h"
#include "ranges.h"


template <class T> class Array2D;

namespace Geometry
{

/*!Class to tesselate part of an array2D data, rrg/crg are given to set the 
  start, step and size of the tesselation. */

mExpClass(Geometry) ArrayTesselator : public ParallelTask
{
public:
    			ArrayTesselator(const float* data,
					int datarowsize,int datacolsize,
					const StepInterval<int>& rrg,
					const StepInterval<int>& crg);

    			ArrayTesselator(const Array2D<float>& data,
					const StepInterval<int>& rrg,
					const StepInterval<int>& crg);

    od_int64		nrIterations() const	
			{ return (rowrange_.nrSteps()+1) * 
			    	 (colrange_.nrSteps()+1); }
    
    			/*<s= 0 for points, 1 for lines, 2 for triangles.*/
    TypeSet<int>	getIndices(char s=2) const
    			{ return s==2 ? stripcis_ : (s ? linecis_ : pointcis_);}
    virtual int		getCoordIndex(int row,int col )	{ return 0; }
    const char* 	message() const	{ return "Tesselating geometry"; }

protected:

    bool			doWork(od_int64 start,od_int64 stop,int);

    int				maxNrThreads() const	{ return 1; }

    const float*		data_;
    int				datarowsize_;
    int				datacolsize_;
    const StepInterval<int>& 	rowrange_;
    const StepInterval<int>& 	colrange_;
   
    TypeSet<int>		pointcis_;
    TypeSet<int>		linecis_;
    TypeSet<int>		stripcis_;
};

}; // namespace

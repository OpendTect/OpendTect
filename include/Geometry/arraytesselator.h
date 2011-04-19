#ifndef arraytesselator_h
#define arraytesselator_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          April 2011
RCS:           $Id: arraytesselator.h,v 1.1 2011-04-19 20:57:18 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "arraynd.h"


template <class T> class Array2D;

namespace Geometry
{

/*!Class to tesselate part of an array2D data, rrg/crg are given to set the 
  start, step and size of the tesselation. */

mClass ArrayTesselator : public ParallelTask
{
public:
    			ArrayTesselator(const Array2D<float>& data,
					const StepInterval<int>& rrg,
					const StepInterval<int>& crg)
			    : data_(data), rowrange_(rrg), colrange_(crg) {}

    od_int64		nrIterations() const	
			{ return rowrange_.nrSteps()*colrange_.nrSteps(); }
    
    			/*<s= 0 for points, 1 for lines, 2 for triangles.*/
    TypeSet<int>	getIndices(char s=2) const
    			{ return s==2 ? stripcis_ : (s ? linecis_ : pointcis_);}
    
    virtual int		getCoordIndex(int row,int col )	{ return 0; }

protected:

    bool			doWork(od_int64 start,od_int64 stop,int);

    const Array2D<float>&	data_;
    const StepInterval<int>& 	rowrange_;
    const StepInterval<int>& 	colrange_;
   
    TypeSet<int>		pointcis_;
    TypeSet<int>		linecis_;
    TypeSet<int>		stripcis_;
};


bool ArrayTesselator::doWork( od_int64 start, od_int64 stop, int )
{
    const int glastrowidx = data_.info().getSize( 0 ) - 1;
    const int glastcolidx = data_.info().getSize( 1 ) - 1;
    const int rowsz = rowrange_.nrSteps();
    const int colsz = colrange_.nrSteps();

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int currow = idx / colsz;
	const int curcol = idx % colsz;
	if ( currow > glastrowidx || curcol > glastcolidx )
	    continue;

	const bool islastrow = currow == glastrowidx;
	const bool islastcol = curcol == glastcolidx;
	const int nextrow = currow + colsz;
	const int nextcol = curcol + 1;

	bool def11 = !mIsUdf( data_.get(currow,curcol) );
	bool def12 = islastcol ? false : !mIsUdf(data_.get(currow,nextcol));
	bool def21 = islastrow ? false : !mIsUdf(data_.get(nextrow,curcol));
	bool def22 = (islastrow || islastcol) ? false : 
	    !mIsUdf(data_.get(nextrow,nextcol));

#define mAddTriangle( ci0, ci1, ci2 ) \
stripcis_ += ci0; \
stripcis_ += ci1; \
stripcis_ += ci2

	const int nrdefined = def11 + def12 + def21 + def22;
	if ( !nrdefined )
	    continue;

	if ( nrdefined==4 )
	{
	    mAddTriangle( idx+colsz, idx+colsz+1, idx );
	    stripcis_ += idx+1;
	    stripcis_ += -1;
	}
	else if ( nrdefined==3 )
	{
	    if ( !def11 )
	    {
		mAddTriangle( idx+colsz, idx+colsz+1, idx+1 );
	    }
	    else if ( !def12 )
	    {
		mAddTriangle( idx, idx+colsz, idx+colsz+1 );
	    }
	    else if ( !def21 )
	    {
		mAddTriangle( idx, idx+colsz+1, idx+1 );
	    }
	    else
	    {
		mAddTriangle( idx, idx+colsz, idx+1 );
	    }
	}
	else if ( def11 )
	{
	    const int prerow = currow - colsz;
	    const int precol = curcol - 1;

	    bool def01 = prerow<0 ? false : !mIsUdf(data_.get(prerow,curcol));
	    bool def10 = precol<0 ? false : !mIsUdf(data_.get(currow,precol));
	    if ( nrdefined==1 )
	    {
		if ( !def01 && def10 )
		    pointcis_ += idx;
	    }
	    else
	    {
		if ( def12 && !def01 )
		{
		    bool def02 = prerow<0 || islastcol ? false :
			!mIsUdf(data_.get(prerow,nextcol));
		    if ( !def02 )
		    {
			linecis_ += idx;
			linecis_ += idx+1;
			linecis_ += -1;
		    }
		}
		else if ( def21 && !def10 )
		{
		    bool def20 = islastrow || precol<0 ? false :
			!mIsUdf(data_.get(nextrow,precol));
		    if ( def20 )
		    {
			linecis_ += idx;
			linecis_ += idx+colsz;
			linecis_ += -1;
		    }
		}
	    }
	}
    }

    return true;
} 


};

#endif

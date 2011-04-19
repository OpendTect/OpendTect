#ifndef arraytesselator_h
#define arraytesselator_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Yuancheng Liu
Date:          April 2011
RCS:           $Id: arraytesselator.h,v 1.2 2011-04-19 21:52:28 cvsyuancheng Exp $
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


#define mGlobleIdx(row,col) row*totalcolsz+col 

#define mAddTriangle( ci0, ci1, ci2 ) \
stripcis_ += ci0; \
stripcis_ += ci1; \
stripcis_ += ci2


bool ArrayTesselator::doWork( od_int64 start, od_int64 stop, int )
{
    const int totalcolsz = data_.info().getSize( 1 );
    const int glastrowidx = data_.info().getSize( 0 ) - 1;
    const int glastcolidx = totalcolsz - 1;
    const int colsz = colrange_.nrSteps();
    const int startidx = rowrange_.start * totalcolsz + colrange_.start;

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int currow = idx / colsz + rowrange_.start;
	const int curcol = idx % colsz + colrange_.start;
	if ( currow > glastrowidx || curcol > glastcolidx )
	    continue;

	const bool islastrow = currow == glastrowidx;
	const bool islastcol = curcol == glastcolidx;
	const int nextrow = currow + totalcolsz;
	const int nextcol = curcol + 1;

	bool def11 = !mIsUdf( data_.get(currow,curcol) );
	bool def12 = islastcol ? false : !mIsUdf(data_.get(currow,nextcol));
	bool def21 = islastrow ? false : !mIsUdf(data_.get(nextrow,curcol));
	bool def22 = (islastrow || islastcol) ? false : 
	    !mIsUdf(data_.get(nextrow,nextcol));
	const int nrdefined = def11 + def12 + def21 + def22;
	if ( !nrdefined )
	    continue;

	const int c11 = mGlobleIdx( currow, curcol );
	const int c12 = mGlobleIdx( currow, nextcol );
	const int c21 = mGlobleIdx( nextrow, curcol );
	const int c22 = mGlobleIdx( nextrow, nextcol );

	if ( nrdefined>2 )
	{
    	    if ( nrdefined==4 )
    	    {
    		mAddTriangle( c21, c22, c11 );
    		stripcis_ += c12;
	    }
	    else if ( !def11 )
	    {
		mAddTriangle( c21, c22, c12 );
	    }
	    else if ( !def12 )
	    {
		mAddTriangle( c21, c22, c11 );
	    }
	    else if ( !def21 )
	    {
		mAddTriangle( c11, c22, c12 );
	    }
	    else
	    {
		mAddTriangle( c11, c21, c12 );
	    }

	    stripcis_ += -1;
	}
	else if ( def11 )
	{
	    const int prerow = currow - totalcolsz;
	    const int precol = curcol - 1;

	    bool def01 = prerow<0 ? false : !mIsUdf(data_.get(prerow,curcol));
	    bool def10 = precol<0 ? false : !mIsUdf(data_.get(currow,precol));
	    if ( nrdefined==1 )
	    {
		if ( !def01 && !def10 )
		    pointcis_ += c11;
	    }
	    else
	    {
		if ( def12 && !def01 )
		{
		    bool def02 = prerow<0 || islastcol ? false :
			!mIsUdf(data_.get(prerow,nextcol));
		    if ( !def02 )
		    {
			linecis_ += c11;
			linecis_ += c12;
			linecis_ += -1;
		    }
		}
		else if ( def21 && !def10 )
		{
		    bool def20 = islastrow || precol<0 ? false :
			!mIsUdf(data_.get(nextrow,precol));
		    if ( !def20 )
		    {
			linecis_ += c11;
			linecis_ += c21;
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

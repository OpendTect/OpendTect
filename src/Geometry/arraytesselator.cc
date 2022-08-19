/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "arraytesselator.h"


namespace Geometry
{

ArrayTesselator::ArrayTesselator( const float* data, int rowsz, int colsz,
				  const StepInterval<int>& rrg,
				  const StepInterval<int>& crg )
    : data_( data )
    , datarowsize_( rowsz )
    , datacolsize_( colsz )
    , rowrange_( rrg )
    , colrange_( crg )
{}


ArrayTesselator::ArrayTesselator( const Array2D<float>& data,
				  const StepInterval<int>& rrg,
				  const StepInterval<int>& crg )
    : data_( data.getData() )
    , datarowsize_( data.info().getSize(0) )
    , datacolsize_( data.info().getSize(1) )
    , rowrange_( rrg )
    , colrange_( crg )
{}


uiString ArrayTesselator::uiNrDoneText() const
{
    return tr("Nodes done");
}


#define mGlobleIdx(row,col) row*datacolsize_+col

#define mAddTriangle( ci0, ci1, ci2 ) \
stripcis_ += ci0; \
stripcis_ += ci1; \
stripcis_ += ci2


bool ArrayTesselator::doWork( od_int64 start, od_int64 stop, int )
{
    const int glastrowidx = datarowsize_ - 1;
    const int glastcolidx = datacolsize_ - 1;
    const int colsz = colrange_.nrSteps()+1;

    for ( od_int64 idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int currow = mCast(int,(idx/colsz)*rowrange_.step +
							    rowrange_.start);
	const int curcol = mCast(int,(idx%colsz)*colrange_.step +
							    colrange_.start);
	if ( currow > glastrowidx || curcol > glastcolidx )
	    continue;

	const bool islastrow = currow == glastrowidx;
	const bool islastcol = curcol == glastcolidx;
	int nextrow = currow + rowrange_.step;
  	if ( nextrow>glastrowidx ) nextrow = glastrowidx;
  	int nextcol = curcol + colrange_.step;
 	if ( nextcol>glastcolidx ) nextcol = glastcolidx;

	const int c11 = mGlobleIdx(currow,curcol);
	const int c12 = mGlobleIdx(currow,nextcol);
	const int c21 = mGlobleIdx(nextrow,curcol);
	const int c22 = mGlobleIdx(nextrow,nextcol);

	bool def11 = !mIsUdf(data_[c11]);
	bool def12 = islastcol ? false : !mIsUdf(data_[c12]);
	bool def21 = islastrow ? false : !mIsUdf(data_[c21]);
	bool def22 = (islastrow || islastcol) ? false : !mIsUdf(data_[c22]);
	const int nrdefined = def11 + def12 + def21 + def22;
	if ( !nrdefined )
	    continue;

	if ( nrdefined>2 )
	{
    	    if ( nrdefined==4 )
    	    {
		mAddTriangle(c21,c22,c11);
		mAddTriangle(c11,c22,c12);
	    }
	    else if ( !def11 )
	    {
		mAddTriangle(c21,c22,c12);
	    }
	    else if ( !def12 )
	    {
		mAddTriangle(c21,c22,c11);
	    }
	    else if ( !def21 )
	    {
		mAddTriangle(c11,c22,c12);
	    }
	    else
	    {
		mAddTriangle(c11,c21,c12);
	    }

	}
	else if ( def11 )
	{
	    const int prerow = currow - rowrange_.step;
	    const int precol = curcol - colrange_.step;

	    const int c01 = mGlobleIdx( prerow, curcol );
    	    const int c10 = mGlobleIdx( currow, precol );
	    bool def01 = prerow<0 ? false : !mIsUdf(data_[c01]);
	    bool def10 = precol<0 ? false : !mIsUdf(data_[c10]);
	    if ( nrdefined==1 )
	    {
		if ( !def01 && !def10 )
		    pointcis_ += c11;
	    }
	    else
	    {
		if ( def12 && !def01 )
		{
		    const int c02 = mGlobleIdx(prerow,nextcol);
		    bool def02 = prerow<0 || islastcol ? false
						       : !mIsUdf(data_[c02]);
		    if ( !def02 )
		    {
			linecis_ += c11;
			linecis_ += c12;
		    }
		}
		else if ( def21 && !def10 )
		{
		    const int c20 = mGlobleIdx(nextrow,precol);
		    bool def20 = islastrow || precol<0 ? false
						       : !mIsUdf(data_[c20]);
		    if ( !def20 )
		    {
			linecis_ += c11;
			linecis_ += c21;
		    }
		}
	    }
	}

	addToNrDone( 1 );
    }

    return true;
}


const TypeSet<int>& ArrayTesselator::arrayIndexes( char s ) const
{
    return s==2 ? stripcis_ : (s==0 ? pointcis_ : linecis_);
}

} // namespace Geometry

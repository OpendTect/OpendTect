/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: binidsurface.cc,v 1.1 2005-01-06 09:45:32 kristofer Exp $";

#include "binidsurface.h"

#include "parametricsurfaceimpl.h"

#include "arrayndimpl.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"


namespace Geometry
{


BinIDSurface::BinIDSurface(const BinIDValue& bv0, const BinIDValue& bv1)
    : depths( 0 )
{
    if ( bv0.binid==bv1.binid )
    {
	pErrMsg("The inital positions may not be at the same binid");
	return;
    }

    step = RowCol(SI().inlStep(true),SI().crlStep(true));

    if ( abs(bv0.binid.inl-bv1.binid.inl)>step.row || 
         abs(bv0.binid.crl-bv1.binid.crl)>step.col || 
	 bv0.binid.inl%step.row || bv0.binid.crl%step.col ||
	 bv1.binid.inl%step.row || bv1.binid.crl%step.col )
    {
	pErrMsg("The inital positions are not valid with regard to the step");
	return;
    }

    origo.row = mMIN(bv0.binid.inl,bv1.binid.inl);
    origo.col = mMIN(bv0.binid.crl,bv1.binid.crl);

    const int inlsize = bv0.binid.inl==bv1.binid.inl ? 1 : 2;
    const int crlsize = bv0.binid.inl==bv1.binid.inl ? 1 : 2;

    depths = new Array2DImpl<float>( inlsize, crlsize );

    for ( int inlidx=0; inlidx<inlsize; inlidx++ )
    {
	for ( int crlidx=0; crlidx<crlsize; crlidx++ )
	{
	    const BinID bid( origo.row+inlidx*step.row,
		    	     origo.col+crlidx*step.col );
	    if ( bid==bv0.binid )
		depths->set( inlidx, crlidx, bv0.value );
	    else if ( bid==bv1.binid )
		depths->set( inlidx, crlidx, bv1.value );
	    else
		depths->set( inlidx, crlidx, mUndefValue );
	}
    }
}


BinIDSurface::~BinIDSurface()
{ delete depths; }


Coord3 BinIDSurface::computePosition( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


Coord3 BinIDSurface::computeNormal( const Coord& ) const
{
    pErrMsg( "Not impl" );
    return Coord3::udf();
}


bool BinIDSurface::insertRow(int row) 
{
    mInsertStart( rowidx, Row, rowIndex(row) );
    mCloneRowVariable( float, depths, computePosition(param).z, mUndefValue )
    return true;
}


bool BinIDSurface::insertColumn(int col) 
{
    mInsertStart( colidx, Col, colIndex(col) );
    mCloneColVariable( float, depths, computePosition(param).z, mUndefValue )
    return true;
}


bool BinIDSurface::removeRow( int row )
{
    pErrMsg( "not implemented ");
    return true;
}


bool BinIDSurface::removeColumn( int col )
{
    pErrMsg( "not implemented ");
    return true;
}


Coord3 BinIDSurface::getKnot( const RCol& rc ) const
{
    const int index = getIndex(rc);
    if ( index==-1 ) return Coord3::udf();

    return Coord3(SI().transform(BinID(rc)), depths->getData()[index]);
}


void BinIDSurface::_setKnot( int idx, const Coord3& np )
{ depths->getData()[idx] = np.z; }


int BinIDSurface::nrRows() const
{ return depths ? depths->info().getSize(rowDim()) : 0; }


int BinIDSurface::nrCols() const
{ return depths ? depths->info().getSize(colDim()) : 0; }


};


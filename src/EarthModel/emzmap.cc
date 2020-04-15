/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/


#include "emzmap.h"

#include "arrayndimpl.h"
#include "od_istream.h"
#include "position.h"
#include "separstr.h"

#include "uistrings.h"


namespace EM
{

ZMapImporter::ZMapImporter( const char* fnm )
    : Executor("Reading ZMap data")
    , fnm_(fnm)
{
    nrdonetxt_ = tr("Positions done");
    istrm_ = new od_istream( fnm_ );
    if ( !initHeader() )
	return;

    data_ = new Array2DImpl<float>( nrrows_, nrcols_ );
    data_->setAll( mUdf(float) );
}


ZMapImporter::~ZMapImporter()
{
    delete istrm_;
    delete data_;
}


bool ZMapImporter::initHeader()
{
    BufferString buf;
    while( true ) // comments
    {
	istrm_->getLine( buf );
	if ( buf.isEmpty() || buf[0]=='!' )
	    continue;
	
	break;
    }

// header line 1
    SeparString ss( buf, ',' );
    if ( ss.size() != 3 )
	return false;
    nrnodesperline_ = toInt( ss[2] );

//header line 2
    istrm_->getLine( ss.rep() );
    if ( ss.size() != 5 )
	return false;
    nrchars_ = toInt( ss[0] );
    undefval_ = toFloat( ss[1] );
    undeftxt_ = ss[2];
    nrdec_ = toInt( ss[3] );
    firstcol_ = toInt( ss[4] );

//header line 3
    istrm_->getLine( ss.rep() );
    if ( ss.size() != 6 )
	return false;
    nrrows_ = toInt( ss[0] );
    nrcols_ = toInt( ss[1] );
    xmin_ = toDouble( ss[2] );
    xmax_ = toDouble( ss[3] );
    ymin_ = toDouble( ss[4] );
    ymax_ = toDouble( ss[5] );

//header line 4
    istrm_->getLine( ss.rep() );

// end
    istrm_->getLine( ss.rep() );

    totalnr_ = nrrows_ * nrcols_;
    dx_ = (xmax_ - xmin_) / (nrcols_-1);
    dy_ = (ymax_ - ymin_) / (nrrows_-1);
    return true;
}


int ZMapImporter::nextStep()
{
    if ( nrdone_ >= totalnr_ )
	return Finished();

    BufferString buf;
    istrm_->getWord( buf );
    const bool doscale = !buf.contains('.');
    const float zval = toFloat( buf );
    if ( mIsUdf(zval) || mIsEqual(zval,undefval_,mDefEps) )
    {
	nrdone_++;
	return MoreToDo();
    }

    // From ZMap format description:
    // grid nodes are stored in column major order. The first column of
    // data is written first, starting at the upper left corner of the grid.
    const int row = (nrrows_-1) - (int)(nrdone_ % nrrows_);
    const int col = (int)(nrdone_ / nrrows_);
    data_->set( row, col, zval );

    nrdone_++;
    return MoreToDo();
}


Coord ZMapImporter::minCoord() const
{ return Coord(xmin_,ymin_); }

Coord ZMapImporter::maxCoord() const
{ return Coord(xmax_,ymax_); }

Coord ZMapImporter::step() const
{ return Coord( dx_, dy_ ); }

} // namespace EM

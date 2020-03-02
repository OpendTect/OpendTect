/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/


#include "emzmap.h"

#include "arrayndimpl.h"
#include "emhorizon3d.h"
#include "od_istream.h"
#include "separstr.h"

#include "uistrings.h"


namespace EM
{

ZMapImporter::ZMapImporter( const char* fnm )
    : Executor("Importing from ZMap")
    , fnm_(fnm)
{
    nrdonetxt_ = tr("Positions done");
    istrm_ = new od_istream( fnm_ );
    if ( !initHeader() )
	return;

    data_ = new Array2DImpl<float>( nrrows_, nrcols_ );
}


ZMapImporter::~ZMapImporter()
{
    delete istrm_;
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
    nrnodesperline_ = ss[2];

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
    return true;
}


int ZMapImporter::nextStep()
{
    BufferString buf;
    istrm_->getWord( buf );

    const int row = (int)(nrdone_ % nrrows_);
    const int col = (int)(nrdone_ / nrrows_);
    data_->set( row, col, toFloat(buf) );
    nrdone_++;
    return MoreToDo();
}

} // namespace EM

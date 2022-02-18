/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2008
________________________________________________________________________

-*/


#include "emhor2dseisiter.h"

#include "emmanager.h"
#include "emhorizon2d.h"
#include "seis2ddata.h"
#include "survinfo.h"
#include "posinfo2dsurv.h"
#include "ioman.h"
#include "ioobj.h"


#define mGetEMObjPtr(mid) EM::EMM().getObject( EM::EMM().getObjectID(mid) )


EM::Hor2DSeisLineIterator::Hor2DSeisLineIterator( const EM::Horizon2D& h2d )
    : nrlines_(0)
{
    init( &h2d );
}


EM::Hor2DSeisLineIterator::Hor2DSeisLineIterator( const MultiID& mid )
    : nrlines_(0)
{
    EM::EMObject* emobj = mGetEMObjPtr( mid );
    mDynamicCastGet(EM::Horizon2D*,h2d,emobj)
    init( h2d );
}


void EM::Hor2DSeisLineIterator::init( const Horizon2D* h2d )
{
    dataset_ = 0;
    curlsid_.setUdf();
    lineidx_ = -1;

    h2d_ = h2d;
    if ( h2d_ )
    {
	h2d_->ref();
	geom_ = &h2d_->geometry();
	const_cast<int&>(nrlines_) = geom_->nrLines();
    }
}


EM::Hor2DSeisLineIterator::~Hor2DSeisLineIterator()
{
    if ( h2d_ )
	h2d_->unRef();
    delete dataset_;
}


bool EM::Hor2DSeisLineIterator::next()
{
    lineidx_++;
    if ( lineidx_ < nrlines_ )
	getLineSet();
    return isValid();
}

bool EM::Hor2DSeisLineIterator::isValid() const
{
    return h2d_ && lineidx_ >= 0 && lineidx_ < nrlines_;
}

void EM::Hor2DSeisLineIterator::reset()
{
    lineidx_ = -1;
}

void EM::Hor2DSeisLineIterator::getLineSet()
{
    if ( !isValid() )
	{ delete dataset_; dataset_ = 0; return; }

    /*Pos::GeomID geomid = geom_->geomID( lineidx_ );
    if ( !dataset_ )
    {
	delete dataset_; dataset_ = 0;
	IOM().to( IOObjContext::Seis );
	IOObj* ioobj = IOM().getLocal( lsnm, 0 );
	if ( ioobj )
	{
	    dataset_ = new Seis2DDataSet( *ioobj );
	    curlsid_ = ioobj->key();
	    delete ioobj;
	}
    }*/
}


const char* EM::Hor2DSeisLineIterator::lineName() const
{
    return geom_->lineName( lineidx_ );
}


int EM::Hor2DSeisLineIterator::lineSetIndex( const char* attrnm ) const
{
    /*if ( !isValid() || !dataset_ ) return -1;

    const LineKey lk( lineName(), attrnm );
    for ( int iln=0; iln<dataset_->nrLines(); iln++ )
    {
	if ( dataset_->lineKey(iln) == lk )
	    return iln;
    }*/

    return -1;
}

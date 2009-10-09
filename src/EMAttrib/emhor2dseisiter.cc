/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: emhor2dseisiter.cc,v 1.1 2009-10-09 08:39:25 cvsbert Exp $";

#include "emhor2dseisiter.h"

#include "emmanager.h"
#include "emhorizon2d.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "ioman.h"
#include "ioobj.h"


#define mGetEMObjPtr(mid) EM::EMM().getObject( EM::EMM().getObjectID(mid) )


EM::Hor2DSeisLineIterator::Hor2DSeisLineIterator( const MultiID& mid )
    : h2d_(0)
    , stickidx_(-1)
    , lset_(0)
    , curlsid_("0")
    , nrsticks_(0)
{
    EM::EMObject* emobj = mGetEMObjPtr( mid );
    mDynamicCastGet(EM::Horizon2D*,h2d,emobj)
    if ( !h2d ) return;
    h2d_ = h2d;
    h2d_->ref();
    geom_ = &h2d_->geometry();
    const_cast<int&>(nrsticks_) = geom_->nrLines();
}


EM::Hor2DSeisLineIterator::~Hor2DSeisLineIterator()
{
    if ( h2d_ )
	h2d_->unRef();
    delete lset_;
}


bool EM::Hor2DSeisLineIterator::next()
{
    stickidx_++;
    if ( stickidx_ < nrsticks_ )
	getLineSet();
    return isValid();
}

bool EM::Hor2DSeisLineIterator::isValid() const
{
    return h2d_ && stickidx_ >= 0 && stickidx_ < nrsticks_;
}

void EM::Hor2DSeisLineIterator::reset()
{
    stickidx_ = -1;
}

void EM::Hor2DSeisLineIterator::getLineSet()
{
    if ( !isValid() )
	{ delete lset_; lset_ = 0; return; }

    const int lineid = geom_->lineID( stickidx_ );
    const MultiID& lsid = geom_->lineSet( geom_->lineID(lineid) );
    if ( !lset_ || lsid != curlsid_ )
    {
	delete lset_; lset_ = 0;
	IOObj* ioobj = IOM().get( lsid );
	if ( ioobj )
	{
	    lset_ = new Seis2DLineSet( *ioobj );
	    curlsid_ = lsid;
	    delete ioobj;
	}
    }
}


int EM::Hor2DSeisLineIterator::lineID() const
{
    return stickidx_ >= 0 ? geom_->lineID(stickidx_) : -1;
}

const char* EM::Hor2DSeisLineIterator::lineName() const
{
    return isValid() ? geom_->lineName( lineID() ) : 0;
}

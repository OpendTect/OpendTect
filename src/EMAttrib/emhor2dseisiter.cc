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
#include "ioobj.h"


#define mGetEMObjPtr(mid) \
    EM::Hor2DMan().getObject( mid )


EM::Hor2DSeisLineIterator::Hor2DSeisLineIterator( const EM::Horizon2D& h2d )
    : nrlines_(0)
{
    init( &h2d );
}


EM::Hor2DSeisLineIterator::Hor2DSeisLineIterator( const DBKey& mid )
    : nrlines_(0)
{
    EM::Object* emobj = mGetEMObjPtr( mid );
    mDynamicCastGet(EM::Horizon2D*,h2d,emobj)
    init( h2d );
}


void EM::Hor2DSeisLineIterator::init( const Horizon2D* h2d )
{
    dataset_ = 0; curlsid_.setInvalid(); lineidx_ = -1;

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

const char* EM::Hor2DSeisLineIterator::lineName() const
{
    return geom_->lineName( lineidx_ );
}


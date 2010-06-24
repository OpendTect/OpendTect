
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
 RCS:		$Id: visvw2ddata.cc,v 1.1 2010-06-24 08:41:01 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2ddata.h"


Vw2DDataObject::Vw2DDataObject()
    : id_( -1 )
    , name_( 0 )
{}


Vw2DDataObject::~Vw2DDataObject()
{ delete name_; }


const char* Vw2DDataObject::name() const
{
    return !name_ || name_->isEmpty() ? 0 : name_->buf();
}


void Vw2DDataObject::setName( const char* nm )
{
    if ( !name_ ) name_ = new BufferString;
    (*name_) = nm;
}

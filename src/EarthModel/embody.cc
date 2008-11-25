/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embody.cc,v 1.3 2008-11-25 15:35:22 cvsbert Exp $";

#include "embody.h"
#include "arraynd.h"
#include "embodytr.h"


EM::ImplicitBody::ImplicitBody() : arr_( 0 ), threshold_( mUdf(float) )	{}

EM::ImplicitBody::~ImplicitBody()
{ delete arr_; }


EM::ImplicitBody* EM::Body::createImplicitBody( TaskRunner* ) const
{ return 0; }


const IOObjContext& EM::Body::getBodyContext() const
{ return EMBodyTranslatorGroup::ioContext(); }

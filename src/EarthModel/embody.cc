/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Sep 2008
 RCS:           $Id: embody.cc,v 1.1 2008-09-04 13:24:00 cvskris Exp $
________________________________________________________________________

-*/

#include "embody.h"
#include "arraynd.h"
#include "embodytr.h"


EM::ImplicitBody::ImplicitBody() : arr_( 0 ), threshold_( mUdf(float) )	{}

EM::ImplicitBody::~ImplicitBody()
{ delete arr_; }


EM::ImplicitBody* EM::Body::createImplicitBody( TaskRunner* ) const
{ return 0; }


EM::Body::Body( EM::EMManager& emm )
    : EMObject( emm )
{}

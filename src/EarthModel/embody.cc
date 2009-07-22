/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embody.cc,v 1.5 2009-07-22 16:01:31 cvsbert Exp $";

#include "embody.h"
#include "embodytr.h"

namespace EM
{

ImplicitBody::ImplicitBody() 
    : arr_( 0 )
    , threshold_( mUdf(float) )	
{}

ImplicitBody::~ImplicitBody()
{ delete arr_; }


ImplicitBody ImplicitBody::operator =( const ImplicitBody& ib )
{
    threshold_ = ib.threshold_;
    inlsampling_ = ib.inlsampling_;
    crlsampling_ = ib.crlsampling_;
    zsampling_ = ib.zsampling_;
    arr_ = ib.arr_;

    return *this;
}


ImplicitBody* Body::createImplicitBody( TaskRunner* ) const
{ return 0; }


const IOObjContext& Body::getBodyContext() const
{ return EMBodyTranslatorGroup::ioContext(); }



}; //end namespace


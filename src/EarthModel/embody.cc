/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embody.cc,v 1.4 2009-02-13 21:18:37 cvsyuancheng Exp $";

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


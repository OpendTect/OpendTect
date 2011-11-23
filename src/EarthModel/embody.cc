/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embody.cc,v 1.7 2011-11-23 17:42:31 cvsyuancheng Exp $";

#include "embody.h"
#include "embodytr.h"
#include "arrayndimpl.h"

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

    delete arr_; arr_ = 0;
    if ( ib.arr_ )
    {
	mDeclareAndTryAlloc( Array3DImpl<float>*, newarr,
		Array3DImpl<float>(ib.arr_->info()) );
	if ( newarr ) 
	{
	    newarr->copyFrom( *ib.arr_ );
	    arr_ = newarr;
	}
    }

    return *this;
}


ImplicitBody* Body::createImplicitBody( TaskRunner*, bool ) const
{ return 0; }


const IOObjContext& Body::getBodyContext() const
{ return EMBodyTranslatorGroup::ioContext(); }



}; //end namespace


/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "embody.h"
#include "embodytr.h"
#include "arrayndimpl.h"

namespace EM
{

ImplicitBody::ImplicitBody() 
    : arr_( 0 )
    , threshold_( mUdf(float) )	
    , cs_( false )				
{}

#define mCopyArr( ib ) \
    if ( ib.arr_ ) \
    { \
	mDeclareAndTryAlloc( Array3DImpl<float>*, newarr, \
		Array3DImpl<float>(ib.arr_->info()) ); \
	if ( newarr )  \
	{ \
	    newarr->copyFrom( *ib.arr_ ); \
	    arr_ = newarr; \
	} \
    }


ImplicitBody::ImplicitBody( const ImplicitBody& ib )
   : threshold_( ib.threshold_ )
   , cs_( ib.cs_ )  
   , arr_( 0 )
{ 
    mCopyArr(ib)
}


ImplicitBody::~ImplicitBody()
{ delete arr_; }


ImplicitBody ImplicitBody::operator =( const ImplicitBody& ib )
{
    threshold_ = ib.threshold_;
    cs_ = ib.cs_;

    delete arr_; arr_ = 0;
    mCopyArr( ib );

    return *this;
}


ImplicitBody* Body::createImplicitBody( TaskRunner*, bool ) const
{ return 0; }


const IOObjContext& Body::getBodyContext() const
{ return EMBodyTranslatorGroup::ioContext(); }



}; //end namespace


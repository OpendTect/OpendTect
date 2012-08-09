/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID mUnusedVar = "$Id: costrans.cc,v 1.12 2012-08-09 06:49:31 cvsaneesh Exp $";


#include "costrans.h"
#include "simpnumer.h"


bool CosineTransform::CosineTransform1D::init()
{
    if ( !isPower( sz_ , 2 ) )
    {
	isfast_ = false;
    }
    else
    {
	isfast_ = true;
	initcosarray();
    }

    two_over_size_ = 2.0f/sz_ ;
    root2_over_rootsize_ = (float) Math::Sqrt( 2.0/sz_ );

    return true;
}


void CosineTransform::CosineTransform1D::initcosarray()
{
    power_ = isPower( sz_ , 2 );

    if ( cosarray_ ) delete [] cosarray_;

    cosarray_ = new float[sz_];

    int hsz=sz_ /2;

    for ( int i=0; i<=hsz-1; i++ ) cosarray_[hsz+i]=4*i+1;

    for ( int group=1; group<=power_-1; group++ )
    {
	int base= 1 << (group-1);
	int nitems = base;
	float factor = 1.0f*(1<<(power_-group));

	for ( int item=1; item<=nitems; item++ )
	{
	    cosarray_[base+item-1] = factor*cosarray_[hsz+item-1];
	}
    }

    for ( int i=1; i<sz_ ; i++ )
	cosarray_[i] = (float) ( 1.0/(2.0*cos(cosarray_[i]*M_PI/(2.0*sz_ ))) );
}

bool CosineTransform::CosineTransform1D::run( bool )
{
    if ( cinput_ && coutput_ )
    {
	for ( int idx=0; idx<nr_; idx++ )
	{
	    int offset = batchstarts_ ? batchstarts_[idx] : idx*batchsampling_;
	    templ_transform1D( cinput_+offset, coutput_+offset, sampling_ );
	}
    }
    else if ( rinput_ && routput_ )
    {
	for ( int idx=0; idx<nr_; idx++ )
	{
	    int offset = batchstarts_ ? batchstarts_[idx] : idx*batchsampling_;
	    templ_transform1D( rinput_+offset, routput_+offset, sampling_ );
	}
    }

    return true;
}


bool CosineTransform::isFast( int sz ) const
{
    return isPower( sz, 2 );
}

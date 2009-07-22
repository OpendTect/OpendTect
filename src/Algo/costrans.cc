/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: costrans.cc,v 1.8 2009-07-22 16:01:29 cvsbert Exp $";


#include "costrans.h"
#include "simpnumer.h"


bool CosineTransform::CosineTransform1D::init()
{
    if ( !isPower( size, 2 ) )
    {
	isfast = false;
    }
    else
    {
	isfast = true;
	initcosarray();
    }

    two_over_size = 2.0/(double)size;
    root2_over_rootsize = Math::Sqrt(2.0/(double)size);

    return true;
}


void CosineTransform::CosineTransform1D::initcosarray()
{
    power = isPower( size, 2 );

    if( cosarray ) delete cosarray;

    cosarray = new float[size];

    int hsz=size/2;

    for ( int i=0; i<=hsz-1; i++ ) cosarray[hsz+i]=4*i+1;

    for ( int group=1; group<=power-1; group++ )
    {
	int base= 1 << (group-1);
	int nitems = base;
	float factor = 1.0*(1<<(power-group));

	for ( int item=1; item<=nitems; item++ )
	{
	    cosarray[base+item-1] = factor*cosarray[hsz+item-1];
	}
    }

    for ( int i=1; i<size; i++ )
	cosarray[i] = 1.0/(2.0*cos(cosarray[i]*M_PI/(2.0*size)));
}

void CosineTransform::CosineTransform1D::transform1D( const float_complex* in,
						      float_complex* out,
						      int space ) const
{
    templ_transform1D( in, out, space );
}


void CosineTransform::CosineTransform1D::transform1D( const float* in,
						      float* out,
						      int space ) const
{
    templ_transform1D( in, out, space );
}


bool CosineTransform::isPossible( int sz ) const
{
    return true;
}


bool CosineTransform::isFast( int sz ) const
{
    return isPower( sz, 2 );
}

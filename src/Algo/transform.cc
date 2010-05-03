/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2001
-*/

static const char* rcsID = "$Id: transform.cc,v 1.8 2010-05-03 15:11:44 cvsyuancheng Exp $";

#include <transform.h>
#include <arraynd.h>


bool GenericTransformND::setInputInfo( const ArrayNDInfo& ni )
{
    if ( !isPossible( ni ) ) return false;

    delete info;
    info = ni.clone();
    return true;
}


const ArrayNDInfo& GenericTransformND::getInputInfo() const
{ return *info; }


bool GenericTransformND::setDir( bool forward_ )
{
    if ( !biDirectional() && forward_==false ) return false;
    forward = forward_;

    return true;
}


bool GenericTransformND::getDir() const
{ return forward; }


bool GenericTransformND::init()
{
    if ( !biDirectional() && forward==false ) return false;
    if ( !info ) return false;

    if ( owntransforms.size() ) deepErase( owntransforms );
    if ( transforms.size() ) transforms.erase();
    
    const int ndim = info->getNDim();

    for ( int idx=0; idx<ndim; idx++ )
    {
	const int sz = info->getSize( idx );

	bool found = false;

	for ( int idy=0; idy<owntransforms.size(); idy++ )
	{
	    if ( owntransforms[idy]->getSize() == sz )
	    {
		transforms += owntransforms[idy];
		found = true;
		break;
	    }
	}

	if ( found ) continue;

	Transform1D* trans = createTransform();
	trans->setSize( sz );
	trans->setDir(forward);
	trans->init();

	owntransforms += trans;
	transforms += trans;
    }

    return true;
}


bool TransformND::isPossible( const ArrayNDInfo& in ) const
{
    const int ndim = in.getNDim();

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !isPossible(in.getSize(idx)) ) return false;
    }

    return true;
}


void TransformND::getNearBigPsblSz( const ArrayNDInfo& in,
					  ArrayNDInfo& out ) const
{
    const int ndim = in.getNDim();

    for ( int idx=0; idx<ndim; idx++ )
	out.setSize(idx,getNearBigPsblSz(in.getSize(idx)));
}


int  TransformND::getNearBigPsblSz( int sz ) const
{
    while ( !isPossible( sz ) ) sz++;
    return sz;
}


bool TransformND::isFast( const ArrayNDInfo& in ) const
{
    const int ndim = in.getNDim();

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !isFast(in.getSize(idx) ) ) return false;
    }

    return true;
}


void TransformND::getNearBigFastSz( const ArrayNDInfo& in,
					  ArrayNDInfo& out ) const
{
    const int ndim = in.getNDim();

    for ( int idx=0; idx<ndim; idx++ )
	out.setSize(idx,getNearBigFastSz(in.getSize(idx)));
}


int  TransformND::getNearBigFastSz( int sz ) const
{
    while ( !isFast( sz ) ) sz++;
    return sz;
}


GenericTransformND::GenericTransformND()
    : forward ( true )
    , info( 0 )
{}


GenericTransformND::~GenericTransformND()
{
    delete info;
    deepErase( owntransforms );
}


bool GenericTransformND::transform( const ArrayND<float>& in,
				    ArrayND<float>& out ) const

{
    if ( !real2real() ) return false;

    if ( out.info() != in.info() || out.info() != *info ) return false;

    const float* ind = in.getData();
    float* outd = out.getData();

    if ( !ind || !outd ) return false;

    const int ndim = info->getNDim();
    if ( ndim==1 )
    {
	transforms[0]->transform1D( ind, outd, 1 );
    }
    else
    {
	transformND( ind, outd, 0 );
    }

    return true;
}


bool GenericTransformND::transform( const ArrayND<float_complex>& in,
				    ArrayND<float_complex>& out ) const

{
    if ( !complex2complex() ) return false;
    if ( out.info() != in.info() || out.info() != *info ) return false;

    const float_complex* ind = in.getData();
    float_complex* outd = out.getData();

    if ( !ind || !outd ) return false;

    const int ndim = info->getNDim();
    if ( ndim==1 )
    {
	transforms[0]->transform1D( ind, outd, 1 );
    }
    else
    {
	transformND( ind, outd, 0 );
    }

    return true;
}


void GenericTransformND::transformND( const float* in, float* out,
				      int dimnr ) const
{
    const int ndim = info->getNDim();
    int nrsmall = info->getSize(dimnr);

    int smallsz = 1;
    for ( int idx=dimnr+1; idx<ndim; idx++ )
	smallsz *= info->getSize(idx);

    int off = 0;
    if ( ndim-dimnr==2 )
    {
	const Transform1D* smalltransform = transforms[ndim-1];
	for ( int idx=0; idx<nrsmall; idx++ )
	{
	    smalltransform->transform1D( in+off, out+off, 1 );
	    off += smallsz;
	}
    }
    else
    {
	for ( int idx=0; idx<nrsmall; idx++ )
	{
	    transformND( in+off, out+off, dimnr+1 );
	    off += smallsz;
	}
    }

    off = 0;
    const Transform1D* smalltransform = transforms[dimnr];
    for ( int idx=0; idx<smallsz; idx++ )
    {
	smalltransform->transform1D( out+off, out+off, smallsz );
	off++;
    }
}


void GenericTransformND::transformND( const float_complex* in,
				      float_complex* out, int dimnr) const
{
    const int ndim = info->getNDim();
    int nrsmall = info->getSize(dimnr);

    int smallsz = 1;
    for ( int idx=1; idx<ndim; idx++ )
	smallsz *= info->getSize(idx);

    int off = 0;
    if ( ndim-dimnr==2 )
    {
	const Transform1D* smalltransform = transforms[ndim-1];
	for ( int idx=0; idx<nrsmall; idx++ )
	{
	    smalltransform->transform1D( in+off, out+off, 1 );
	    off += smallsz;
	}
    }
    else
    {
	for ( int idx=0; idx<nrsmall; idx++ )
	{
	    transformND( in+off, out+off, dimnr+1 );
	    off += smallsz;
	}
    }

    off = 0;
    const Transform1D* smalltransform = transforms[dimnr];
    for ( int idx=0; idx<smallsz; idx++ )
    {
	smalltransform->transform1D( out+off, out+off, smallsz );
	off++;
    }
}


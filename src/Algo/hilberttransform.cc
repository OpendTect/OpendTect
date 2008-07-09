/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		June 2008
 RCS:		$Id: hilberttransform.cc,v 1.1 2008-07-09 12:19:55 cvsnanne Exp $
________________________________________________________________________

-*/


#include "hilberttransform.h"

#include "arrayndimpl.h"


HilbertTransform::HilbertTransform()
    : info_(0)
    , forward_(true)
{}


HilbertTransform::~HilbertTransform()
{
    delete info_;
}


bool HilbertTransform::setInputInfo( const ArrayNDInfo& in )
{
    // TODO
    return true;
}


bool HilbertTransform::isPossible( int sz ) const
{
    // TODO
    return true;
}


bool HilbertTransform::init()
{
    // TODO
    return true;
}


bool HilbertTransform::transform( const ArrayND<float>& in,
				  ArrayND<float>& out ) const
{
    // TODO
    return true;
}


bool HilbertTransform::transform( const ArrayND<float_complex>& in,
				  ArrayND<float_complex>& out ) const
{
    // TODO
    return true;
}

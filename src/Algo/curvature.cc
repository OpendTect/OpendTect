/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/
 
static const char* rcsID = "$Id: curvature.cc,v 1.3 2009/07/22 16:01:29 cvsbert Exp $";


#include "curvature.h"
#include "math2.h"
#include "math.h"
#include "simpnumer.h"

Curvature::Setup::Setup()
    : mean_( false )
    , gaussian_( false )
    , minmax_( false )
    , mostposneg_( false )
    , shapeindex_( false )
    , dip_( false )
    , strike_( false )
    , contour_( false )
    , curvedness_( false )
{}


Curvature::Curvature( const Setup& setup )
    : setup_( setup )
    , mean_( mUdf(double) )
    , gaussian_( mUdf(double) )
    , max_( mUdf(double) )
    , min_( mUdf(double) )
    , mostpos_( mUdf(double) )
    , mostneg_( mUdf(double) )
    , shapeindex_( mUdf(double) )
    , dip_( mUdf(double) )
    , strike_( mUdf(double) )
    , contour_( mUdf(double) )
    , curvedness_( mUdf(double) )
{}


bool Curvature::set( double z1, double z2, double z3,
		     double z4, double z5, double z6,
		     double z7, double z8, double z9,
		     double dist0, double dist1,
		     bool checkforudfs )
{
    if ( checkforudfs )
    {
	if ( mIsUdf(z1) || mIsUdf(z2) || mIsUdf(z3) ||
	     mIsUdf(z4) || mIsUdf(z5) || mIsUdf(z6) ||
	     mIsUdf(z7) || mIsUdf(z8) || mIsUdf(z9) )
	    return false;
    }

    const double dist0_2 = dist0*dist0;
    const double dist1_2 = dist1*dist1;

    const double a = ((z1+z3+z4+z6+z7+z9)/2-(z2+z5+z8)) / (3*dist0_2);
    const double b = ((z1+z2+z3+z7+z8+z9)/2-(z4+z5+z6)) / (3*dist1_2);
    const double c = (z3+z7-z1-z9) / (4*dist0*dist1);
    const double d = (z3+z6+z9-z1-z4-z7) / (6*dist0);
    const double e = (z1+z2+z3-z7-z8-z9) / (6*dist1);

    const double c2 = c*c;
    const double d2 = d*d;
    const double e2 = e*e;

    const double cde = c*d*e;

    static double powval = 3./2;

    const bool dominmax = setup_.minmax_ || setup_.shapeindex_ ||
			  setup_.curvedness_;

    if ( setup_.mean_ || dominmax )
	mean_ = (a*(1+e2)+b*(1+d2)-cde) / pow((1+d2+e2),powval);

    if ( setup_.gaussian_ || dominmax )
	gaussian_ = (4*a*b-c2) / intpow(1+d2+e2,2);

    if ( dominmax )
    {
	const double tmp = Math::Sqrt( mean_*mean_ - gaussian_ );

	//This is not identical to Robert's article, as K_max should be the 
	//the value that is furtherst away from zero, and K_min should be the
	//value that is closest to zero. 8th May 2009, KT.
    
	max_ = mean_;
	min_ = mean_;
	if ( mean_>=0 )
	{
	    max_ += tmp;
	    min_ -= tmp;
	}
	else
	{
	    max_ -= tmp;
	    min_ += tmp;
	}
    }

    if ( setup_.mostposneg_ )
    {
	const double tmp = Math::Sqrt( intpow(a-b,2) + c2 );
	mostpos_ = a+b+tmp;
	mostneg_ = a+b-tmp;
    }

    if ( setup_.shapeindex_ )
    {
	shapeindex_ = 2/M_PI * atan2( min_+max_, max_-min_ );
    }

    if ( setup_.dip_ || setup_.strike_ || setup_.contour_ )
    {
	const double e2plusd2 = e2+d2;
	const bool nocalc = mIsZero( e2plusd2, mDefEps );
	
	if ( setup_.dip_ )
	{
	    dip_ = nocalc
		? 0
		: 2*(a*d2+b*e2+cde)/((e2plusd2)*pow((1+e2plusd2),powval));
	}

	if ( setup_.strike_ )
	{
	    strike_ = nocalc
		? 0
		: 2*(a*e2+b*d2-cde)/((e2plusd2)*Math::Sqrt(1+e2plusd2));
	}

	if ( setup_.contour_ )
	{
	    contour_ = nocalc ? 0 : 2*(a*e2+b*d2-cde)/pow(e2plusd2,powval);
	}
    }

    if ( setup_.curvedness_ )
    {
	curvedness_ = Math::Sqrt( (max_*max_+ min_*min_)/2 );
    }

    return true;
}

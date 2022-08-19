/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "dragcontroller.h"
#include "timefun.h"


DragController::DragController()
    : slowmaxdragtomouseratio_( 0.5 )	/* A value of 0.5 guarantees step-one
					   positioning in any mouse direction */
    , fastmaxdragtomouseratio_( 50.0 )
    , slowmindragtomouseratio_( 0.1 )
    , fastmindragtomouseratio_( 1.0 )
    , slowmousespeed_( 50 )
    , fastmousespeed_( 500 )
    , linearbackdragfrac_( 0.5 )
{
    init( mUdf(Coord) );
}


DragController::~DragController()
{}


void DragController::init( const Coord& mousepos, double scalefactor,
			   const Coord3& dragdir )
{
    initmousepos_ = mousepos;
    scalefactor_ = scalefactor>0.0 ? scalefactor : 1.0;
    dragdir_ = dragdir;
    screendragmode_ = false;
    screendragprojvec_ = Coord( 0.0, 0.0 );
    screendragfactor_ = 1.0;
    prevdragtime_ = Time::getMilliSeconds();

    reInit( mousepos );
}


void DragController::reInit( const Coord& mousepos )
{
    prevmousepos_ = mousepos;
    prevdragval_ = 0.0;
    prevtransval_ = 0.0;
    maxdragval_ = 0.0;
    dragsign_ = 0;
}


void DragController::setFastMaxDragToMouseRatio( float ratio )
{
    if ( ratio > 0.0 )
    {
	fastmaxdragtomouseratio_ = ratio;
	if ( ratio < fastmindragtomouseratio_ )
	    setFastMinDragToMouseRatio( ratio );
	if ( ratio < slowmaxdragtomouseratio_ )
	    setSlowMaxDragToMouseRatio( ratio );
    }
}


float DragController::getFastMaxDragToMouseRatio() const
{ return fastmaxdragtomouseratio_; }


void DragController::setSlowMaxDragToMouseRatio( float ratio )
{
    if ( ratio > 0.0 )
    {
	slowmaxdragtomouseratio_ = ratio;
	if ( ratio < slowmindragtomouseratio_ )
	    setSlowMinDragToMouseRatio( ratio );
	if ( ratio > fastmaxdragtomouseratio_ )
	    setFastMaxDragToMouseRatio( ratio );
    }
}


float DragController::getSlowMaxDragToMouseRatio() const
{ return slowmaxdragtomouseratio_; }


void DragController::setFastMinDragToMouseRatio( float ratio )
{
    if ( ratio > 0.0 )
    {
	fastmindragtomouseratio_ = ratio;
	if ( ratio > fastmaxdragtomouseratio_ )
	    setFastMaxDragToMouseRatio( ratio );
	if ( ratio < slowmindragtomouseratio_ )
	    setSlowMinDragToMouseRatio( ratio );
    }
}


float DragController::getFastMinDragToMouseRatio() const
{ return fastmindragtomouseratio_; }


void DragController::setSlowMinDragToMouseRatio( float ratio )
{
    if ( ratio > 0.0 )
    {
	slowmindragtomouseratio_ = ratio;
	if ( ratio > slowmaxdragtomouseratio_ )
	    setSlowMaxDragToMouseRatio( ratio );
	if ( ratio > fastmindragtomouseratio_ )
	    setFastMinDragToMouseRatio( ratio );
    }
}


float DragController::getSlowMinDragToMouseRatio() const
{ return slowmindragtomouseratio_; }


void DragController::setFastMouseSpeed( float speed )
{
    if ( speed > 0.0 )
    {
	fastmousespeed_ = speed;
	if ( speed < slowmousespeed_ )
	    slowmousespeed_ = speed;
    }
}


float DragController::getFastMouseSpeed() const
{ return fastmousespeed_; }


void DragController::setSlowMouseSpeed( float speed )
{
    if ( speed > 0.0 )
    {
	slowmousespeed_ = speed;
	if ( speed > fastmousespeed_ )
	    fastmousespeed_ = speed;
    }
}


float DragController::getSlowMouseSpeed() const
{ return slowmousespeed_; }


void DragController::setLinearBackDragFrac( float fraction )
{
    if ( fraction>=0.0 && fraction<=1.0 )
	linearbackdragfrac_ = fraction;
}


float DragController::getLinearBackDragFrac() const
{ return linearbackdragfrac_; }


void DragController::dragInScreenSpace( bool fromstart, const Coord& projvec )
{
    if ( projvec.sqAbs() )
    {
	screendragmode_ = fromstart;
	screendragprojvec_ = projvec;
    }
}


void DragController::manageScreenDragging( double& dragval,
					   const Coord& mousepos )
{
    /* If the 3D drag direction is (almost) perpendicular to the view plane,
       failing model inversion causes dragger values to become inconsistent
       with mouse movement sooner or later. From that moment on, or already
       from the start, screen drag mode will instead derive dragger values
       from 2D mouse positions. */

    if ( !screendragprojvec_.sqAbs() )
	return;

    const double prevproj = screendragprojvec_.dot(prevmousepos_-initmousepos_);
    const double stepproj = screendragprojvec_.dot( mousepos-prevmousepos_ );

    if ( !screendragmode_ && dragsign_*dragval<prevdragval_ &&
	 stepproj*prevproj>0.0 && dragsign_*prevproj>0.0 )
    {
	screendragmode_ = true;
	screendragfactor_ = (float) (dragsign_*prevdragval_ / prevproj);
    }

    if ( screendragmode_ )
    {
	const double curproj = screendragprojvec_.dot(mousepos-initmousepos_);

	if ( prevproj*curproj < 0.0 )
	{
	    screendragfactor_ = 1.0;
	    const double frac = fabs( prevproj / (curproj-prevproj) );
	    prevdragval_ = fabs( curproj * frac/(1.0-frac) );
	}

	dragval = screendragfactor_ * curproj;
    }
}


double DragController::absTransform( double dragval, const Coord& mousepos,
				     double maxdragdist )
{
    if ( dragval < 0.0 )
	return mUdf(double);

    if ( mousepos==prevmousepos_ || dragval==prevdragval_ )
	return prevtransval_;

    float delay = (float) Time::passedSince( prevdragtime_ );
    if ( delay <= 0.0 )
	delay = 0.5;

    const double deltamouse = mousepos.distTo( prevmousepos_ );
    const double mousespeed = deltamouse * 1000.0 / delay;

    double frac = 0.0;
    if ( mousespeed >= fastmousespeed_ )
	frac = 1.0;
    else if ( mousespeed>slowmousespeed_ )
	frac = (mousespeed-slowmousespeed_) / (fastmousespeed_-slowmousespeed_);

    const double maxdragtomouseratio = fastmaxdragtomouseratio_*frac +
				       slowmaxdragtomouseratio_*(1.0-frac);
    const double mindragtomouseratio = fastmindragtomouseratio_*frac +
				       slowmindragtomouseratio_*(1.0-frac);

    double deltadrag = dragval - prevdragval_;

    const double maxdeltadrag = deltamouse * maxdragtomouseratio;
    const double mindeltadrag = deltamouse * mindragtomouseratio;

    if ( fabs(deltadrag) > maxdeltadrag )
	deltadrag = deltadrag>=0.0 ? maxdeltadrag : -maxdeltadrag;
    if ( fabs(deltadrag) < mindeltadrag )
	deltadrag = deltadrag>=0.0 ? mindeltadrag : -mindeltadrag;

    double transval = prevtransval_ + deltadrag;

    if ( dragval < prevdragval_ )
    {
	if ( !mIsUdf(maxdragdist) && prevtransval_>fabs(maxdragdist) )
	{
	    prevtransval_ = fabs(maxdragdist);
	    transval = prevtransval_ + deltadrag;
	}

	const double linearval = prevtransval_ * dragval/prevdragval_;
	if ( transval<linearval || dragval<linearbackdragfrac_*maxdragval_ )
	    transval = linearval;
    }
    else
	maxdragval_ = dragval;

    prevdragval_ = dragval;
    prevtransval_ = transval;
    prevmousepos_ = mousepos;

    prevdragtime_ = Time::getMilliSeconds();

    return transval;
}


void DragController::transform( double& realdragval, const Coord& mousepos,
				double maxdragdist )
{
    if ( mIsUdf(initmousepos_) )
	init( mousepos );

    double dragval = realdragval / scalefactor_;

    manageScreenDragging( dragval, mousepos );

    if ( dragval*dragsign_ < 0.0 )
    {
	const double frac = prevdragval_ / (fabs(dragval)+prevdragval_);
	prevdragtime_ += (int) floor( frac * Time::passedSince(prevdragtime_) );
	reInit( prevmousepos_*(1.0-frac) + mousepos*frac );
    }

    if ( dragval )
	dragsign_ = dragval<0.0 ? -1 : 1;

    if ( !mIsUdf(maxdragdist) )
	maxdragdist /= scalefactor_;

    realdragval = dragsign_ * scalefactor_ *
		  absTransform( fabs(dragval), mousepos, maxdragdist );
}


void DragController::transform( Coord3& dragvec, const Coord& mousepos,
				double maxdragdist )
{
    if ( mIsUdf(initmousepos_) )
	init( mousepos );

    if ( !dragdir_.sqAbs() )
	dragdir_ = dragvec;

    const Coord3 normalizeddragdir = dragdir_.normalize();

    const int dragsign = normalizeddragdir.dot(dragvec)<0.0 ? -1 : 1;
    double dragval = dragsign * dragvec.abs();

    transform( dragval, mousepos, maxdragdist );

    dragvec = dragval * normalizeddragdir;
}

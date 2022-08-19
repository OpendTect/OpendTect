#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "coord.h"


/*!
\brief Auxiliary class to control dragger speed. The idea is that slowly
moving the mouse forces the dragger to move only one inl/crl/z per (few)
pixel(s), while moving it fast can have it cover many lines per pixel.
*/

mExpClass(Algo) DragController
{
public:
			DragController();
			~DragController();

    void		init(const Coord& mousepos,double scalefactor=1.0,
			     const Coord3& dragdir=Coord3(0.0,0.0,0.0));

    void		setFastMaxDragToMouseRatio(float distanceperpixel);
    float		getFastMaxDragToMouseRatio() const;

    void		setSlowMaxDragToMouseRatio(float distanceperpixel);
    float		getSlowMaxDragToMouseRatio() const;

    void		setFastMinDragToMouseRatio(float distanceperpixel);
    float		getFastMinDragToMouseRatio() const;

    void		setSlowMinDragToMouseRatio(float distanceperpixel);
    float		getSlowMinDragToMouseRatio() const;

    void		setFastMouseSpeed(float pixelspersecond);
    float		getFastMouseSpeed() const;

    void		setSlowMouseSpeed(float pixelspersecond);
    float		getSlowMouseSpeed() const;

    void		setLinearBackDragFrac(float fraction);
    float		getLinearBackDragFrac() const;
			/*!<If moving back towards initial position, 'fraction'
			defines where the dragger starts acting linear to reach
			its origin at the same time as the mouse pointer. */

    void		transform(double& dragval,const Coord& mousepos,
				  double maxdragdist=mUdf(double));
    void		transform(Coord3& dragvec,const Coord& mousepos,
				  double maxdragdist=mUdf(double));

    void		dragInScreenSpace(bool fromstart,const Coord& projvec);

protected:

    void		reInit(const Coord& mousepos);

    void		manageScreenDragging(double& dragval,
					     const Coord& mousepos);

    double		absTransform(double dragval,const Coord& mousepos,
				     double maxdragdist=mUdf(double));

    float		fastmaxdragtomouseratio_;
    float		slowmaxdragtomouseratio_;
    float		fastmindragtomouseratio_;
    float		slowmindragtomouseratio_;
    float		fastmousespeed_;
    float		slowmousespeed_;
    float		linearbackdragfrac_;

    Coord		initmousepos_;
    Coord3		dragdir_;
    int			prevdragtime_;

    bool		screendragmode_;
    float		screendragfactor_;
    Coord		screendragprojvec_;

    Coord		prevmousepos_;
    double		prevdragval_;
    double		prevtransval_;
    double		maxdragval_;
    int			dragsign_;
    double		scalefactor_;
};

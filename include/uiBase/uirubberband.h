#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigeom.h"

mFDQtclass(QMouseEvent)
mFDQtclass(QRubberBand)
mFDQtclass(QWidget)

mExpClass(uiBase) uiRubberBand
{
public:
			uiRubberBand(mQtclass(QWidget*) p)
			    : parent_(p)
			    , qrubberband_(0)
			    , aspectratio_(mUdf(float))    {}
			~uiRubberBand();

    void		start(mQtclass(QMouseEvent*));
    void		extend(mQtclass(QMouseEvent*));
    void		stop(mQtclass(QMouseEvent*));

    uiPoint		origin() const		{ return origin_; }
    uiRect		area() const		{ return area_; }

    void		setAspectRatio( float ar )	{ aspectratio_ = ar; }
    float		getAspectRatio() const		{ return aspectratio_; }

protected:

    mQtclass(QRubberBand*)	qrubberband_;
    mQtclass(QWidget*)		parent_;

    uiPoint		origin_;
    uiRect		area_;

    float		aspectratio_;

    void		handleEv(mQtclass(QMouseEvent*),bool);
};


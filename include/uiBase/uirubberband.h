#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			uiRubberBand(QWidget* p);
			~uiRubberBand();

    void		start(mQtclass(QMouseEvent*));
    void		extend(mQtclass(QMouseEvent*));
    void		stop(mQtclass(QMouseEvent*));

    uiPoint		origin() const		{ return origin_; }
    uiRect		area() const		{ return area_; }

    void		setAspectRatio( float ar )	{ aspectratio_ = ar; }
    float		getAspectRatio() const		{ return aspectratio_; }

protected:

    mQtclass(QRubberBand*)	qrubberband_	= nullptr;
    mQtclass(QWidget*)		parent_;

    uiPoint			origin_;
    uiRect			area_;

    float			aspectratio_		= mUdf(float);

    void			handleEv(mQtclass(QMouseEvent*),bool);
};

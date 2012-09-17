#ifndef uirubberband_h
#define uirubberband_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
 RCS:		$Id: uirubberband.h,v 1.5 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigeom.h"

class QMouseEvent;
class QRubberBand;
class QWidget;

mClass uiRubberBand
{
public:
    			uiRubberBand(QWidget* p)
			    : parent_(p)
			    , qrubberband_(0)	{}
			~uiRubberBand();

    void		start(QMouseEvent*);
    void		extend(QMouseEvent*);
    void		stop(QMouseEvent*);

    uiPoint		origin() const		{ return origin_; }
    uiRect		area() const		{ return area_; }

protected:

    QRubberBand*	qrubberband_;
    QWidget*		parent_;

    uiPoint		origin_;
    uiRect		area_;

    void		handleEv(QMouseEvent*,bool);
};

#endif

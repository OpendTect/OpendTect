#ifndef uirubberband_h
#define uirubberband_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		November 2006
 RCS:		$Id: uirubberband.h,v 1.3 2007-03-29 16:13:36 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigeom.h"

class QMouseEvent;
class QRubberBand;
class QWidget;

class uiRubberBand
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

#ifndef uirubberband_h
#define uirubberband_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		November 2006
 RCS:		$Id: uirubberband.h,v 1.6 2012-08-03 13:00:53 cvskris Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uigeom.h"

class QMouseEvent;
class QRubberBand;
class QWidget;

mClass(uiBase) uiRubberBand
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


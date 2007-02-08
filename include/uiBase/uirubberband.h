#ifndef uirubberband_h
#define uirubberband_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		November 2006
 RCS:		$Id: uirubberband.h,v 1.1 2007-02-08 21:26:42 cvsnanne Exp $
________________________________________________________________________

-*/


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
    void		move(QMouseEvent*);
    void		stop(QMouseEvent*);

protected:

    QRubberBand*	qrubberband_;
    QWidget*		parent_;
};

#endif

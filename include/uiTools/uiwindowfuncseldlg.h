#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		August 2007
 RCS:		$Id: uiwindowfuncseldlg.h,v 1.2 2008-03-06 04:37:43 cvssatyaki Exp $
________________________________________________________________________

-*/



#include "uidialog.h"
#include "color.h"

class uiCanvas;
class uiWorld2Ui;

/*!brief Displays a windowfunction. */

class uiWindowFuncSelDlg : public uiDialog
{

public:
			uiWindowFuncSelDlg(uiParent*);
			~uiWindowFuncSelDlg();

    void		setCurrentWindowFunc(const char*);

protected:
				
    void		reDraw(CallBacker*);

    void		setColor(const Color&);
    const Color&	getColor() const;
    uiCanvas*		canvas_;
    uiWorld2Ui*		transform_;
    Color		color_;
    TypeSet<uiPoint>	pointlist_;

};

#endif

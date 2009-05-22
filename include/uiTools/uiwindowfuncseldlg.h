#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		August 2007
 RCS:		$Id: uiwindowfuncseldlg.h,v 1.7 2009-05-22 04:35:46 cvssatyaki Exp $
________________________________________________________________________

-*/



#include "uidialog.h"
#include "color.h"

class uiAxisHandler;
class uiGraphicsItemGroup;
class uiGraphicsView;
class uiGenInput;
class uiListBox;
class uiRectItem;
class uiWorld2Ui;
class WindowFunction;

/*!brief Displays a windowfunction. */

mClass uiWindowFuncSelDlg : public uiDialog
{

public:
			uiWindowFuncSelDlg(uiParent*,const char*,float);
			~uiWindowFuncSelDlg();

    void		setCurrentWindowFunc(const char*,float);
    bool		getCurrentWindowName(BufferString&);
    void		setVariable(float); 
    float		getVariable();
    void		createLine(const WindowFunction&,bool replace=false);

protected:
				
    void		draw();
    void		taperSelChg(CallBacker*);
    void		variableChanged(CallBacker*);
    //bool		rejectOK(CallBacker*);

    uiGraphicsItemGroup* polyitemgrp_;
    uiGraphicsView*	view_;
    uiGenInput*		varinpfld_;
    uiListBox*		taperlistfld_;
    uiWorld2Ui*		transform_;
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;
    uiRectItem*		borderrectitem_;
    float		variable_;
    TypeSet< TypeSet<uiPoint> >	pointlistset_;
    TypeSet<Color>		linesetcolor_;
    ObjectSet<WindowFunction>	winfunc_;

};

#endif

#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2007
 RCS:		$Id: uiwindowfuncseldlg.h,v 1.9 2009-09-11 13:17:28 cvsbruno Exp $
________________________________________________________________________

-*/



#include "uidialog.h"
#include "uigroup.h"
#include "color.h"
#include "mathfunc.h"

class uiAxisHandler;
class uiGraphicsItemGroup;
class uiGraphicsView;
class uiGenInput;
class uiListBox;
class uiRectItem;
class uiWorld2Ui;
class WindowFunction;

/*!brief Displays a mathfunction. */

mClass uiFuncSelDraw : public uiGroup
{

public:
			uiFuncSelDraw(uiParent*,const char*);
			~uiFuncSelDraw();

    Notifier<uiFuncSelDraw> funclistselChged;

    void		addFunction(FloatMathFunction*); 
    void		addToList(const char*); 
    void		addToListAsCurrent(const char*); 
    const char*		getCurrentListName() const;
    int			getCurrentListSize() const; 
    void		createLine(const FloatMathFunction&);
    
    void		funcSelChg(CallBacker*);

protected:
				
    uiGraphicsItemGroup* polyitemgrp_;
    uiGraphicsView*	view_;
    uiListBox*		funclistfld_;
    uiWorld2Ui*		transform_;
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;
    uiRectItem*		borderrectitem_;
    TypeSet< TypeSet<uiPoint> >	pointlistset_;
    TypeSet<Color>		linesetcolor_;
    ObjectSet<FloatMathFunction> mathfunc_;
    
    void		draw();

};


/*!brief Displays a windowfunction. */
mClass uiWindowFuncSelDlg : public uiDialog
{

public:
			uiWindowFuncSelDlg(uiParent*,const char*,float);
			~uiWindowFuncSelDlg();

    const char*		getCurrentWindowName() const;
    void		setCurrentWindowFunc(const char*,float);
    void		setVariable(float); 
    float		getVariable();

protected:
				
    //bool		rejectOK(CallBacker*);
    uiGenInput*		varinpfld_;
    float		variable_;
    uiFuncSelDraw*	funcdrawer_;
    ObjectSet<WindowFunction>	winfunc_;
    
    WindowFunction*	getCurrentWindowFunc();
    void		funcSelChg(CallBacker*);

};



#endif

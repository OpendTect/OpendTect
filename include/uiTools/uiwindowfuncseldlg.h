#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		August 2007
 RCS:		$Id: uiwindowfuncseldlg.h,v 1.4 2008-05-22 10:46:27 cvssatyaki Exp $
________________________________________________________________________

-*/



#include "uidialog.h"
#include "color.h"

class uiAxisHandler;
class uiCanvas;
class uiGenInput;
class uiListBox;
class uiWorld2Ui;
class WindowFunction;

/*!brief Displays a windowfunction. */

class uiWindowFuncSelDlg : public uiDialog
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
				
    void		reDraw(CallBacker*);
    void		taperSelChg(CallBacker*);
    void		variableChanged(CallBacker*);
    //bool		rejectOK(CallBacker*);

    uiCanvas*		canvas_;
    uiGenInput*		varinpfld_;
    uiListBox*		taperlistfld_;
    uiWorld2Ui*		transform_;
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;
    float		variable_;
    TypeSet< TypeSet<uiPoint> >	pointlistset_;
    TypeSet<Color>		linesetcolor_;
    ObjectSet<WindowFunction>	winfunc_;

};

#endif

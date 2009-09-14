#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2007
 RCS:		$Id: uiwindowfuncseldlg.h,v 1.12 2009-09-14 15:06:12 cvsbruno Exp $
________________________________________________________________________

-*/



#include "uidialog.h"
#include "uigroup.h"
#include "bufstringset.h"
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

    mStruct Setup
    {
			Setup()
			    : xaxrg_(-1.2,1.2,0.25)
			    , yaxrg_(0,1,0.25) {}
					      

	mDefSetupMemb(StepInterval<float>,xaxrg)			      
	mDefSetupMemb(StepInterval<float>,yaxrg)	
	mDefSetupMemb(const char*,name)	
    };	

			uiFuncSelDraw(uiParent*,const Setup&);
			~uiFuncSelDraw();

    Notifier<uiFuncSelDraw> funclistselChged;

    void		addFunction(FloatMathFunction*); 
    void		addToList(const char*);
    int			getListSize() const;
    int			getNrSel() const;
    void		setAsCurrent(const char*); 
    int			removeLastItem(); 
    void		createLine(const FloatMathFunction&);
    const char*		getCurrentListName() const;
    void		getSelectedItems(TypeSet<int>&) const; 
    bool		isSelected(int) const;
    
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

    const char*		getCurrentWindowName() const;
    void		setCurrentWindowFunc(const char*,float);
    void		setVariable(float); 
    float		getVariable();

protected:
				
    //bool		rejectOK(CallBacker*);
    BufferStringSet	funcnames_;
    uiGenInput*		varinpfld_;
    float		variable_;
    uiFuncSelDraw*	funcdrawer_;
    ObjectSet<WindowFunction>	winfunc_;
    
    WindowFunction* 	getWindowFuncByName(const char*); 
    void		funcSelChg(CallBacker*);

};



#endif

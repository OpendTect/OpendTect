#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2007
 RCS:		$Id: uiwindowfuncseldlg.h,v 1.23 2009-10-23 13:06:27 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "uifunctiondisplay.h"
#include "uigroup.h"
#include "bufstringset.h"
#include "color.h"
#include "mathfunc.h"

class uiAxisHandler;
class uiGraphicsItemGroup;
class uiGenInput;
class uiListBox;
class uiRectItem;
class uiWorld2Ui;
class WindowFunction;

/*!brief Displays a mathfunction. */



mClass uiFunctionDrawer : public uiFunctionDisplay
{

public:
    mStruct Setup
    {
			Setup()
			    : xaxrg_(-1.2,1.2,0.25)
			    , yaxrg_(0,1,0.25) 
			    , border_(10,10,10,10)		       
			    , funcrg_(-1.2,1.2) 
			    , xaxname_("")	       
			    , yaxname_("")
			    , fillbelow_(false)
			    , drawownaxis_(true)
			    {}
					      
	mDefSetupMemb(StepInterval<float>,xaxrg)
	mDefSetupMemb(StepInterval<float>,yaxrg)	
	mDefSetupMemb(uiBorder,border)	
	mDefSetupMemb(const char*,name)	
	mDefSetupMemb(BufferString,xaxname)	
	mDefSetupMemb(BufferString,yaxname)	
	mDefSetupMemb(Interval<float>,funcrg)	
	mDefSetupMemb(bool,fillbelow)	
	mDefSetupMemb(bool,drawownaxis)	
    };

			uiFunctionDrawer(uiParent*,const Setup&);
			~uiFunctionDrawer();
    
    void		addColor(const Color& col) { linesetcolor_ += col; }
    void		draw(TypeSet<int>&);
    void		setFrame();
    void		createLine(const FloatMathFunction*);
    void		erasePoints() { pointlistset_.erase(); }

    void 		setFunctionRange(Interval<float>& rg) { funcrg_ = rg; }
    Interval<float>& 	getFunctionRange() { return funcrg_; }
    

protected:

    Interval<float>  	funcrg_;
    float		variable_;
    uiWorld2Ui*		transform_;
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;
    uiRectItem*		borderrectitem_;
    uiGraphicsItemGroup* polyitemgrp_;
    TypeSet< TypeSet<uiPoint> >	pointlistset_;
    TypeSet<Color>	linesetcolor_;
};


mClass uiFuncSelDraw : public uiGroup
{
public:

			uiFuncSelDraw(uiParent*,const uiFunctionDrawer::Setup&);

    Notifier<uiFuncSelDraw> funclistselChged;

    void		addFunction(FloatMathFunction*); 
    void		addToList(const char*,bool withcolor=true);
    int			getListSize() const;
    int			getNrSel() const;
    const char*		getCurrentListName() const;
    void		getSelectedItems(TypeSet<int>&) const; 
    bool		isSelected(int) const;
    void		removeItem(int); 
    int			removeLastItem(); 
    void		setAsCurrent(const char*); 
    void		setSelected(int);
    void		setFunctionRange(Interval<float>);
    void		setAxisRange(Interval<float>);

    void		funcSelChg(CallBacker*);

protected:
				
    uiFunctionDrawer*	view_;
    uiListBox*		funclistfld_;
    ObjectSet<FloatMathFunction> mathfunc_;
};


/*!brief Displays a windowfunction. */
mClass uiWindowFuncSelDlg : public uiDialog
{

public:

			uiWindowFuncSelDlg(uiParent*,const char*,float);

    void		funcSelChg(CallBacker*);
    const char*		getCurrentWindowName() const;
    void		setCurrentWindowFunc(const char*,float);
    void		setVariable(float); 
    float		getVariable();

protected:
				
    BufferStringSet	funcnames_;
    float		variable_;
    bool		isfrequency_;
    uiGenInput*		varinpfld_;
    uiFuncSelDraw*	funcdrawer_;
    ObjectSet<WindowFunction>	winfunc_;
    
    WindowFunction* 	getWindowFuncByName(const char*); 
};


mClass uiFreqTaperDlg : public uiDialog
{
public:

    mStruct Setup
    {
			Setup()
			    : hasmin_(false)
			    , hasmax_(true)
			    , displayfac_(true)
			    , winfreqsize_(100)			
			    {}

	mDefSetupMemb(const char*,name);	
	mDefSetupMemb(bool,hasmin)	
	mDefSetupMemb(bool,hasmax)	
	mDefSetupMemb(float,displayfac)	
	mDefSetupMemb(Interval<float>,minfreqrg)	
	mDefSetupMemb(Interval<float>,maxfreqrg)	
	mDefSetupMemb(Interval<float>,orgfreqrg)	
	mDefSetupMemb(int,winfreqsize)	
    };

			uiFreqTaperDlg(uiParent*,const Setup&);
    
    void		setVariables(Interval<float>); 
    Interval<float>	getVariables() const; 

    mStruct DrawData 
    {
	float		variable_;
	Interval<float> freqrg_;
	Interval<float> funcrg_;
	Interval<float> xaxrg_;
	Interval<float> orgfreqrg_;
    };

    DrawData		dd1_;
    DrawData		dd2_;

protected:

    uiGenInput*		varinpfld_;
    uiGenInput*		freqinpfld_;
    uiGenInput*		freqrgfld_;
    	
    ObjectSet<uiFunctionDrawer> drawers_;
    WindowFunction*	winfunc_;

    bool		hasmin_;
    bool		hasmax_;
    bool		isminactive_;
    float 		dispfac_;
    int 		winfsize_;

    float		getSlope();
    float 		getPercentsFromSlope(float);
    void 		setViewRanges();

    void 		setFreqFromPercents(CallBacker*);
    void 		setPercentsFromFreq(CallBacker*);

    void		freqChoiceChged(CallBacker*);
    void 		getFromScreen(CallBacker*);
    void 		putToScreen(CallBacker*);
    void		taperChged(CallBacker*);
};
#endif

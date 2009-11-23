#ifndef uiwindowfuncseldlg_h
#define uiwindowfuncseldlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Nov 2009
 RCS:		$Id: uifreqtaper.h,v 1.1 2009-11-23 15:59:22 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uifunctiondisplay.h"
#include "uiwindowfunctionsel.h"
#include "uibutton.h"
#include "arrayndutils.h"
#include <arrayndimpl.h>

class uiGenInput;
class uiFuncTaperDisp;
class uiSliceSelDlg;
class uiSliderExtra;

class ArrayNDWindow;
class CubeSampling;


mClass uiFreqTaperDlg : public uiDialog
{
public:

    mStruct Setup
    {
			Setup()
			    : hasmin_(false)
			    , hasmax_(true)
			    , seisnm_(0)		   
			    {}

	mDefSetupMemb(const char*,name);	
	mDefSetupMemb(const char*,seisnm);	
	mDefSetupMemb(bool,hasmin)	
	mDefSetupMemb(bool,hasmax)	
	mDefSetupMemb(Interval<float>,minfreqrg)	
	mDefSetupMemb(Interval<float>,maxfreqrg)	
    };

			uiFreqTaperDlg(uiParent*,const Setup&);
			~uiFreqTaperDlg();
    
    void		setFreqRange(Interval<float>); 
    Interval<float>	getFreqRange() const; 

    mStruct DrawData 
    {
			DrawData()
			    : variable_(0)
			    {}

	float		variable_;
	Interval<float> freqrg_;
	Interval<float> reffreqrg_;
	float		slope_;
    };

    DrawData		dd1_;
    DrawData		dd2_;

protected:

    uiGenInput*		varinpfld_;
    uiGenInput*		freqinpfld_;
    uiGenInput*		inffreqfld_;
    uiGenInput*		supfreqfld_;
    uiSliderExtra*	sliderfld_;
    uiPushButton*	previewfld_;
    uiSliceSelDlg*	posdlg_;
    CubeSampling*	cs_;
    Array1DImpl<float>* funcvals_; 
    	
    uiFuncTaperDisp*    drawer_;

    const char*		seisnm_;
    bool		hasmin_;
    bool		hasmax_;
    bool		isminactive_;
    int 		datasz_;

    void		setSlopeFromFreq();
    void 		setPercentsFromFreq();
    void 		setFreqFromSlope(float);

    void		freqChoiceChged(CallBacker*);
    void 		freqChanged(CallBacker*);
    void 		sliderChanged(CallBacker*);
    void		previewPushed(CallBacker*);
    void 		putToScreen(CallBacker*);
    void		taperChged(CallBacker*);
    void		slopeChanged(CallBacker*);
};



mClass uiFuncTaperDisp : public uiFunctionDisplay
{
public:

    mStruct Setup : public uiFunctionDisplay::Setup
    {
			Setup()
			    : is2sided_(false)
			    {}

	mDefSetupMemb(int,datasz);	
	mDefSetupMemb(const char*,name);	
	mDefSetupMemb(const char*,xaxnm);	
	mDefSetupMemb(const char*,yaxnm);	
	mDefSetupMemb(Interval<float>,leftrg)	
	mDefSetupMemb(Interval<float>,rightrg)
	mDefSetupMemb(bool,is2sided);	
    };

			uiFuncTaperDisp(uiParent*,const Setup&);
			~uiFuncTaperDisp();
    
    mStruct WinData
    {
			WinData()
			    : window_(0)  
			    {}

	Interval<float> rg_;
	ArrayNDWindow*	window_;
	int 		winsz_;
	float		paramval_;
    };

    WinData		leftd_;
    WinData		rightd_;

    void 		setWindows(float,float rightvar=0);
    void		setFunction(Array1DImpl<float>&,Interval<float>);
    			
    float*		getWinValues() const { return window_->getValues(); } 
    float*		getFuncValues() const { return funcvals_->getData(); } 
    
    void		taperChged(CallBacker*);

protected:

    ArrayNDWindow*	window_;

    Array1DImpl<float>* funcvals_; 
    Array1DImpl<float>* orgfuncvals_;
    Interval<float>	funcrg_;	

    bool		isfunction_;
    bool		is2sided_;
    int 		datasz_;
};


mClass uiFreqTaperSel : public uiWindowFunctionSel
{
public:
				uiFreqTaperSel(uiParent*,const Setup&);

    void                        setIsMinMaxFreq(bool,bool);
    void                        setInputFreqValue(float,int);
    Interval<float>             freqValues() const;
    void                        setRefFreqs(Interval<float>);

    protected :

    Interval<float>             freqrg_;
    Interval<float>             selfreqrg_;
    bool                        isminfreq_;
    bool                        ismaxfreq_;
    uiFreqTaperDlg*             freqtaperdlg_;
    const char*                 seisnm_;

    void                        winfuncseldlgCB(CallBacker*);
    void                        windowClosed(CallBacker*);
    void                        setSelFreqs(CallBacker*);
};

#endif

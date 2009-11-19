#ifndef uiseiswvltattr_h
#define uiseiswvltattr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltattr.h,v 1.11 2009-11-19 15:00:17 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uislider.h"

class ArrayNDWindow;
class Wavelet;
class uiCheckBox;
class uiFuncTaperDisp;
class uiGenInput;
class uiWaveletDispProp;
class WaveletAttrib;

template <class T> class Array1DImpl;

mClass uiSeisWvltSliderDlg : public uiDialog 
{
public:
				~uiSeisWvltSliderDlg();

    Notifier<uiSeisWvltSliderDlg> acting;
    const Wavelet*              getWavelet() const  { return wvlt_; }

    WaveletAttrib*		wvltattr_;
    uiSliderExtra*		sliderfld_;
    Wavelet* 			wvlt_;
    const Wavelet* 		orgwvlt_;

    virtual void		act(CallBacker*) {}
    void			constructSlider(uiSliderExtra::Setup&,
	    					const Interval<float>&);
protected:
				uiSeisWvltSliderDlg(uiParent*,Wavelet&);
};


mClass uiSeisWvltRotDlg : public uiSeisWvltSliderDlg 
{
public:
				uiSeisWvltRotDlg(uiParent*,Wavelet&);
protected:

    void			act(CallBacker*);
};


mClass uiSeisWvltTaperDlg : public uiSeisWvltSliderDlg 
{
public:
				uiSeisWvltTaperDlg(uiParent*,Wavelet&);
				~uiSeisWvltTaperDlg();
protected: 
    
    bool			isfreqtaper_;
    int				wvltsz_;

    Array1DImpl<float>* 	wvltvals_;
    Array1DImpl<float>* 	freqvals_;
    Array1DImpl<float>* 	spectrum_;
    uiWaveletDispProp*		properties_;
    uiFuncTaperDisp*		timedrawer_;
    uiFuncTaperDisp*		freqdrawer_;
    uiGenInput*			typefld_;
    uiCheckBox*			mutefld_;

    void			setFreqData();
    void			setTimeData();

    void			act(CallBacker*);
};



mClass uiWaveletDispProp : public uiGroup
{
public:

    mStruct Setup
    {
				Setup()
				: withphase_(true)
				, withgridlines_(true) 
				{}

	mDefSetupMemb(bool,withphase)
	mDefSetupMemb(bool,withgridlines)
    };

				uiWaveletDispProp(uiParent*,const Wavelet&,
						const Setup&);
				~uiWaveletDispProp();

    void                        setAttrCurves(const Wavelet&);
    uiFuncTaperDisp*		getAttrDisp(int idx)
    				{ return attrdisps_[idx]; }
    Interval<float>             getFreqRange() const { return freqrange_; }
    Interval<float>             getTimeRange() const { return timerange_; }

private:

    int                         wvltsz_;
    const char*			attrnms_[4];
    ObjectSet<uiFuncTaperDisp>  attrdisps_;
    ObjectSet< Array1DImpl<float> > attrarrays_;
    
    Interval<float>		timerange_;
    Interval<float>		freqrange_;

    void			addAttrDisp(bool);
};


mClass uiWaveletDispPropDlg : public uiDialog
{
public:
				uiWaveletDispPropDlg(uiParent*,const Wavelet&);
				~uiWaveletDispPropDlg();
protected:

    uiWaveletDispProp*		properties_;
};

#endif

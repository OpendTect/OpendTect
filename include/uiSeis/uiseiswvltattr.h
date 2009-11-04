#ifndef uiseiswvltattr_h
#define uiseiswvltattr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltattr.h,v 1.9 2009-11-04 14:33:47 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "hilberttransform.h"
#include "uislider.h"

class ArrayNDWindow;
class FFT;
class WindowFunction;
class Wavelet;
class uiCheckBox;
class uiFunctionDisplay;
class uiWaveletDispProp;

template <class T> class Array1DImpl;
class WaveletAttrib
{
public:
				WaveletAttrib(const Wavelet*);
				~WaveletAttrib();

    void 			getHilbert(Array1DImpl<float>&);
    void 			getPhase(Array1DImpl<float>&);
    void 			getFrequency(Array1DImpl<float>&, 
					     bool ispad=true);
    void 			muteZeroFrequency(Array1DImpl<float>&);

protected:

    HilbertTransform*		hilbert_;
    FFT*			fft_;
    int                         wvltsz_;
    Array1DImpl<float>*  	wvltarr_;
};


mClass uiSeisWvltSliderDlg : public uiDialog 
{
public:
				uiSeisWvltSliderDlg(uiParent*,Wavelet*);
				~uiSeisWvltSliderDlg();

    Notifier<uiSeisWvltSliderDlg> acting;
    const Wavelet*              getWavelet() const  { return wvlt_; }

public :
    WaveletAttrib*		wvltattr_;
    uiSliderExtra*		sliderfld_;
    Wavelet* 			wvlt_;
    const Wavelet* 		orgwvlt_;

    virtual void		act(CallBacker*) {}
    void			constructSlider(uiSliderExtra::Setup&,
	    					const Interval<float>);
};


mClass uiSeisWvltRotDlg : public uiSeisWvltSliderDlg 
{
public:
				uiSeisWvltRotDlg(uiParent*,Wavelet*);
protected:

    void			act(CallBacker*);
};


mClass uiSeisWvltTaperDlg : public uiSeisWvltSliderDlg 
{
public:
				uiSeisWvltTaperDlg(uiParent*,Wavelet*);
				~uiSeisWvltTaperDlg();

protected: 

    ArrayNDWindow* 		window_;
    Array1DImpl<float>* 	wvltvals_;
    Array1DImpl<float>* 	orgwvltvals_;
    uiWaveletDispProp*		properties_;
    uiFunctionDisplay*		drawer_;
    uiCheckBox*			mutefld_;

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

				uiWaveletDispProp(uiParent*,const Wavelet*,
						const Setup&);
				~uiWaveletDispProp();

    void                        setAttrCurves(const Wavelet*);
    uiFunctionDisplay*		getAttrDisp(int idx)  
    				{ return attrdisps_[idx]; }

private:

    int                         wvltsz_;
    const char*			attrnms_[4];
    ObjectSet<uiFunctionDisplay> attrdisps_;
    ObjectSet< Array1DImpl<float> > attrarrays_;

    void			addAttrDisp(bool);
};


mClass uiWaveletDispPropDlg : public uiDialog
{
public:
				uiWaveletDispPropDlg(uiParent*,const Wavelet*);
				~uiWaveletDispPropDlg();
protected:

    uiWaveletDispProp*		properties_;
};

#endif

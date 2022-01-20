#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uislider.h"
#include "uistring.h"

class ArrayNDWindow;
class Wavelet;
class uiCheckBox;
class uiFuncTaperDisp;
class uiFuncDispBase;
class uiFreqTaperGrp;
class uiGenInput;
class uiWaveletDispProp;
class WaveletAttrib;

template <class T> class Array1DImpl;

mExpClass(uiSeis) uiSeisWvltSliderDlg : public uiDialog
{ mODTextTranslationClass(uiSeisWvltSliderDlg);
public:
				~uiSeisWvltSliderDlg();

    Notifier<uiSeisWvltSliderDlg> acting;
    const Wavelet*              getWavelet() const  { return wvlt_; }

protected:
				uiSeisWvltSliderDlg(uiParent*,Wavelet&);

    WaveletAttrib*		wvltattr_;
    uiSlider*			sliderfld_;
    Wavelet*			wvlt_;
    const Wavelet*		orgwvlt_;

    virtual void		act(CallBacker*) {}
    void			constructSlider(uiSlider::Setup&,
						const Interval<float>&);
};


mExpClass(uiSeis) uiSeisWvltRotDlg : public uiSeisWvltSliderDlg
{ mODTextTranslationClass(uiSeisWvltRotDlg);
public:
				uiSeisWvltRotDlg(uiParent*,Wavelet&);
protected:

    void			act(CallBacker*);
};


mExpClass(uiSeis) uiSeisWvltTaperDlg : public uiSeisWvltSliderDlg
{ mODTextTranslationClass(uiSeisWvltTaperDlg);
public:
				uiSeisWvltTaperDlg(uiParent*,Wavelet&);
				~uiSeisWvltTaperDlg();
protected:

    bool			isfreqtaper_;
    int				wvltsz_;

    Array1DImpl<float>*		wvltvals_;
    Array1DImpl<float>*		freqvals_;
    Interval<float>		timerange_;
    Interval<float>		freqrange_;

    uiFuncTaperDisp*		timedrawer_;
    uiFuncTaperDisp*		freqdrawer_;
    uiFreqTaperGrp*		freqtaper_;

    uiGenInput*			typefld_;
    uiCheckBox*			mutefld_;

    void			setFreqData();
    void			setTimeData();

    void			act(CallBacker*);
    void			typeChoice(CallBacker*);
};



mExpClass(uiSeis) uiWaveletDispProp : public uiGroup
{ mODTextTranslationClass(uiWaveletDispProp);
public:

				uiWaveletDispProp(uiParent*,const Wavelet&);
				~uiWaveletDispProp();

    void                        setAttrCurves(const Wavelet&);
    Interval<float>             getFreqRange() const { return freqrange_; }
    Interval<float>             getTimeRange() const { return timerange_; }

private:

    int                         wvltsz_;
    ObjectSet<uiFuncDispBase>	attrdisps_;
    ObjectSet< Array1DImpl<float> > attrarrays_;

    WaveletAttrib*		wvltattr_;

    Interval<float>		timerange_;
    Interval<float>		freqrange_;

    void			addAttrDisp(int);

};


mExpClass(uiSeis) uiWaveletDispPropDlg : public uiDialog
{ mODTextTranslationClass(uiWaveletDispPropDlg);
public:
				uiWaveletDispPropDlg(uiParent*,const Wavelet&);
				~uiWaveletDispPropDlg();
protected:

    uiWaveletDispProp*		properties_;
};


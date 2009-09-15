#ifndef uiseiswvltattr_h
#define uiseiswvltattr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltattr.h,v 1.4 2009-09-15 13:36:21 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "hilberttransform.h"

template <class T> class Array1DImpl;
class FFT;
class Wavelet;
class uiComboBox;
class uiSliderExtra;
class uiFunctionDisplay;

mClass uiSeisWvltRotDlg : public uiDialog 
{
public:
				uiSeisWvltRotDlg(uiParent*,Wavelet*);
				~uiSeisWvltRotDlg();

    Notifier<uiSeisWvltRotDlg>	phaserotating;
    const Wavelet*              getWavelet() const  { return wvlt_; }

protected:

    void			rotatePhase(float);

    void			sliderMove(CallBacker*);


    uiSliderExtra*		sliderfld_;
    Wavelet* 			wvlt_;
    Wavelet* 			orgwvlt_;
    HilbertTransform* 		hilbert_;
};


class uiWaveletDispPropDlg : public uiDialog
{
public:
				uiWaveletDispPropDlg(uiParent*,const Wavelet*);
				~uiWaveletDispPropDlg();

protected:

    uiComboBox*                 attrfld_;
    int                         wvltsz_;
    FFT*                        fft_;
    const Wavelet*              wvlt_;
    ObjectSet<uiFunctionDisplay> attrdisps_;
    ObjectSet< Array1DImpl<float> > attrarrays_;

    void			addAttrDisp(bool);
    void                        setAttrArrays();
    void                        setDispCurves();
    void 			attrSel(CallBacker*);
};


#endif

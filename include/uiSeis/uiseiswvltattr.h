#ifndef uiseiswvltattr_h
#define uiseiswvltattr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltattr.h,v 1.7 2009-09-24 07:33:44 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "hilberttransform.h"

class Wavelet;
class uiSliderExtra;
class uiFunctionDisplay;
class FFT;

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

protected:

    HilbertTransform*		hilbert_;
    FFT*			fft_;
    int                         wvltsz_;
    Array1DImpl<float>*  	wvltarr_;

};


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


    WaveletAttrib*		wvltattr_;
    uiSliderExtra*		sliderfld_;
    Wavelet* 			wvlt_;
    const Wavelet* 		orgwvlt_;
};


mClass uiWaveletDispPropDlg : public uiDialog
{
public:
				uiWaveletDispPropDlg(uiParent*,const Wavelet*);
				~uiWaveletDispPropDlg();

protected:

    int                         wvltsz_;
    const Wavelet*              wvlt_;
    WaveletAttrib*		wvltattr_;
    ObjectSet<uiFunctionDisplay> attrdisps_;
    ObjectSet< Array1DImpl<float> > attrarrays_;

    void			addAttrDisp(bool);
    void                        setAttrCurves();
};

#endif

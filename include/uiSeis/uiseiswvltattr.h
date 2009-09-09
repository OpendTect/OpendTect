#ifndef uiseiswvltattr_h
#define uiseiswvltattr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltattr.h,v 1.3 2009-09-09 13:55:51 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "hilberttransform.h"

class Wavelet;
class uiSliderExtra;

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


#endif

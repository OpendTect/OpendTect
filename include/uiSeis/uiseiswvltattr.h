#ifndef uiseiswvltattr_h
#define uiseiswvltattr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltattr.h,v 1.1 2009-09-09 07:46:05 cvsbruno Exp $
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

    void			sliderMove(CallBacker*);

protected:

    void			rotatePhase(float);

    uiSliderExtra*		sliderfld_;
    Wavelet* 			wvlt_;
    HilbertTransform* 		hilbert_;
};


#endif

#ifndef uiwelltiewavelet_h
#define uiwelltiewavelet_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          January 2009
RCS:           $Id: uiwelltiewavelet.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"

template <class T> class Array1DImpl;
class CtxtIOObj;
class FFT;
class Wavelet;

class uiFlatViewer;
class uiFunctionDisplay;
class uiGenInput;
class uiToolButton;
class uiIOObjSel;
class uiTextEdit;

namespace WellTie
{

class GeoCalculator;
class DataHolder;
class Setup;
class uiWaveletDispPropDlg;
class uiWavelet;

mClass uiWaveletView : public uiGroup
{
public:

	    uiWaveletView(uiParent*,DataHolder*); 
	    ~uiWaveletView();

    void 			initWavelets();

    Notifier<uiWaveletView> 	activewvltChged;

protected:

    WellTie::DataHolder*	dataholder_;
    CtxtIOObj&          	wvltctio_;

    uiGenInput*			activewvltfld_;
    ObjectSet<WellTie::uiWavelet> uiwvlts_;

    void 			createWaveletFields(uiGroup*);	   
    void 			activeWvltChanged(CallBacker*);
};


class uiWavelet : public uiGroup
{

public: 
    				uiWavelet(uiParent*,Wavelet*);
				~uiWavelet();

protected:				    

    Wavelet*			wvlt_; 	
    ObjectSet<uiToolButton>     wvltbuts_;
    uiFlatViewer*               viewer_;

    void			initWaveletViewer();
    void			drawWavelet();

    void			rotatePhase(CallBacker*);
};				


}; //namespace WellTie
#endif


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

class CtxtIOObj;
class FFT;
class Wavelet;

class uiFlatViewer;
class uiFunctionDisplay;
class uiGenInput;
class uiToolButton;
class uiIOObjSel;
class uiTextEdit;
class uiWaveletDispPropDlg;

namespace WellTie
{

class GeoCalculator;
class DataHolder;
class Setup;
class uiWavelet;

mClass uiWaveletView : public uiGroup
{
public:

	    uiWaveletView(uiParent*,DataHolder*); 
	    ~uiWaveletView();

    void 			initWavelets();

    Notifier<uiWaveletView> 	activeWvltChged;
    void 			activeWvltChanged(CallBacker*);

protected:

    WellTie::DataHolder*	dataholder_;
    CtxtIOObj&          	wvltctio_;

    uiGenInput*			activewvltfld_;
    ObjectSet<WellTie::uiWavelet> uiwvlts_;

    void 			createWaveletFields(uiGroup*);	   
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
    uiWaveletDispPropDlg*  	wvltpropdlg_;
    uiParent*			par_;

    void			initWaveletViewer();
    void			drawWavelet();

    void			rotatePhase(CallBacker*);
    void 			dispProperties(CallBacker*);
};				


}; //namespace WellTie
#endif


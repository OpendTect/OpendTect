#ifndef uiwelltiewavelet_h
#define uiwelltiewavelet_h

/*+
________________________________________________________________________

CopyRight:     (C) dGB Beheer B.V.
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
class WellTieGeoCalculator;
class WellTieDataHolder;
class WellTieSetup;
class Wavelet;

class uiFlatViewer;
class uiFunctionDisplay;
class uiPushButton;
class uiIOObjSel;
class uiTextEdit;
class uiWellTieWaveletDispDlg;

mClass uiWellTieWaveletView : public uiGroup
{
public:

	    uiWellTieWaveletView(uiParent*,const WellTieDataHolder*); 
	    ~uiWellTieWaveletView();

    void 		initWavelets();
    //Notifier<uiWellTieWaveletView>      wvltChanged;

protected:

    const WellTieDataHolder*	dataholder_;
    const WellTieSetup&		twtss_;
    CtxtIOObj&          	wvltctio_;

    ObjectSet<uiPushButton> 	wvltbuts_;
    ObjectSet<uiFlatViewer> 	viewer_;	   
    uiIOObjSel*			wvltfld_;
    uiTextEdit*			infofld_;
    ObjectSet<const Wavelet>  	wvlts_;
    uiWellTieWaveletDispDlg*    wvltestdlg_;
    uiWellTieWaveletDispDlg*    wvltinitdlg_;

    void			initWaveletViewer(int);
    void 			createWaveletFields(uiGroup*);	   
    void			drawWavelet(const Wavelet*,int);
    
    void			saveWvltPushed(CallBacker*);
    void 			viewInitWvltPropPushed(CallBacker*);
    void 			viewEstWvltPropPushed(CallBacker*);
    void 			wvtSel(CallBacker*);
};



class uiWellTieWaveletDispDlg : public uiDialog
{
public:
			    uiWellTieWaveletDispDlg(uiParent*,const Wavelet*,
				    		const WellTieDataHolder*);
			    ~uiWellTieWaveletDispDlg();

    void				setValArrays();
    void                                setDispCurves();

protected:

    ObjectSet< Array1DImpl<float> >     proparrays_;
    
    int                                 wvltsz_;
    CtxtIOObj&                          wvltctio_;
    FFT*				fft_;
    ObjectSet<uiFunctionDisplay>        wvltdisps_;
    const Wavelet*			wvlt_;
    WellTieGeoCalculator&		geocalc_;

};

#endif


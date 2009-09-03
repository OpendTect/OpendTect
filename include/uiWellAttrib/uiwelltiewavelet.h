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
class uiPushButton;
class uiIOObjSel;
class uiTextEdit;

namespace WellTie
{

class GeoCalculator;
class DataHolder;
class Setup;
class uiWaveletDispDlg;

mClass uiWaveletView : public uiGroup
{
public:

	    uiWaveletView(uiParent*,const DataHolder*); 
	    ~uiWaveletView();

    void 		initWavelets();

protected:

    const WellTie::DataHolder*	dataholder_;
    const WellTie::Setup&	twtss_;
    CtxtIOObj&          	wvltctio_;

    ObjectSet<uiPushButton> 	wvltbuts_;
    ObjectSet<uiFlatViewer> 	viewer_;	   
    uiIOObjSel*			wvltfld_;
    uiTextEdit*			infofld_;
    ObjectSet<const Wavelet>  	wvlts_;
    WellTie::uiWaveletDispDlg*  wvltestdlg_;
    WellTie::uiWaveletDispDlg*  wvltinitdlg_;

    void			initWaveletViewer(int);
    void 			createWaveletFields(uiGroup*);	   
    void			drawWavelet(const Wavelet*,int);
    
    void			saveWvltPushed(CallBacker*);
    void 			viewInitWvltPropPushed(CallBacker*);
    void 			viewEstWvltPropPushed(CallBacker*);
    void 			wvtSel(CallBacker*);
};



class uiWaveletDispDlg : public uiDialog
{
public:
			    uiWaveletDispDlg(uiParent*,const Wavelet*,
				    		const WellTie::DataHolder*);
			    ~uiWaveletDispDlg();

    void				setValArrays();
    void                                setDispCurves();

protected:

    ObjectSet< Array1DImpl<float> >     proparrays_;
    
    int                                 wvltsz_;
    CtxtIOObj&                          wvltctio_;
    FFT*				fft_;
    ObjectSet<uiFunctionDisplay>        wvltdisps_;
    const Wavelet*			wvlt_;
    WellTie::GeoCalculator&		geocalc_;

};

}; //namespace WellTie
#endif


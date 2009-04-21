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

#include "uigroup.h"

class CtxtIOObj;
class WellTieSetup;
class Wavelet;

class uiFlatViewer;
class uiIOObjSel;
class uiTextEdit;

mClass uiWellTieWavelet : public uiGroup
{
public:

	    uiWellTieWavelet(uiParent*,WellTieSetup&); 
	    ~uiWellTieWavelet();

    void 		initWavelets(Wavelet*);
    Notifier<uiWellTieWavelet>      wvltChanged;

protected:

    WellTieSetup&	twtss_;
    CtxtIOObj&          wvltctio_;

    ObjectSet<uiFlatViewer> viewer_;	   
    uiIOObjSel*		wvltfld_;
    uiTextEdit*		infofld_;

    void		initWaveletViewer(const int);
    void 		createWaveletFields(uiGroup*);	   
    void		drawWavelet(Wavelet*,const int);
    
    void 		wvtSel(CallBacker*);
};


#endif







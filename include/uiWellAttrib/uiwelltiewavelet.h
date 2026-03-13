#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uigroup.h"
#include "uistring.h"

class FlatDataPack;
class Wavelet;
class uiFlatViewer;
class uiFunctionDisplay;
class uiGenInput;
class uiToolButton;
class uiWaveletDispPropDlg;

namespace WellTie
{

class uiWavelet;

mExpClass(uiWellAttrib) uiWaveletView : public uiGroup
{ mODTextTranslationClass(uiWaveletView);
public:

				uiWaveletView(uiParent*,ObjectSet<Wavelet>&);
				~uiWaveletView();

    void			redrawWavelets();
    void			setActiveWavelet(bool initial);
    bool			isInitialWvltActive() const;

    Notifier<uiWaveletView>	activeWvltChgd;

protected:

    ObjectSet<Wavelet>&		wvltset_;

    uiGenInput*			activewvltfld_;
    ObjectSet<uiWavelet>	uiwvlts_;

    void			createWaveletFields(uiGroup*);

    void			activeWvltChangedCB(CallBacker*);
};


mClass(uiWellAttrib) uiWavelet : public uiGroup
{ mODTextTranslationClass(uiWavelet);

public:
				uiWavelet(uiParent*,Wavelet*,bool);
				~uiWavelet();

    Notifier<uiWavelet>		wvltChged;
    void			drawWavelet();
    void			setAsActive(bool);

protected:

    bool			isactive_;

    Wavelet*			wvlt_;
    ObjectSet<uiToolButton>     wvltbuts_;
    uiFlatViewer*               viewer_;
    uiWaveletDispPropDlg*	wvltpropdlg_ = nullptr;
    RefMan<FlatDataPack>	fdp_;

    void			initWaveletViewer();

    void			dispProperties(CallBacker*);
    void			rotatePhase(CallBacker*);
    void			taperCB(CallBacker*);
    void			wvltChangedCB(CallBacker*);
};

} // namespace WellTie

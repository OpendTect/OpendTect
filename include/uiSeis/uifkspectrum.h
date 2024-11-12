#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "datapack.h"
#include "odcomplex.h"
#include "uiflatviewmainwin.h"

namespace Fourier { class CC; }
template <class T> class Array2D;
namespace FlatView { class AuxData; }
class FlatDataPack;
class VolumeDataPack;
class uiGenInput;
class uiToolButton;

mExpClass(uiSeis) uiFKSpectrum : public uiFlatViewMainWin
{ mODTextTranslationClass(uiFKSpectrum);
public:
				uiFKSpectrum(uiParent*,bool setbp=false);
				~uiFKSpectrum();

    bool			setDataPack(const VolumeDataPack&,
					    int version=0);
    mDeprecated("Use setDataPack")
    bool			setDataPackID(const DataPackID&,
					      const DataPackMgr::MgrID&,
					      int version=0);
    bool			setData(const Array2D<float>&);

    float			getMinValue() const;
    float			getMaxValue() const;

private:

    bool			initFFT(int,int);
    bool			compute(const Array2D<float>&);
    bool			view(Array2D<float>&);
    FlatView::AuxData*		initAuxData();
    void			setVelCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			mousePressCB(CallBacker*);

    Fourier::CC*		fft_		= nullptr;
    Array2D<float_complex>*	input_		= nullptr;
    Array2D<float_complex>*	output_		= nullptr;
    Array2D<float>*		spectrum_	= nullptr;

    FlatView::AuxData*		lineitm_;
    FlatView::AuxData*		minvelitm_	= nullptr;
    FlatView::AuxData*		maxvelitm_	= nullptr;
    RefMan<FlatDataPack>	vddp_;

    uiGenInput*			ffld_;
    uiGenInput*			kfld_;
    uiGenInput*			velfld_;

    uiGenInput*			minfld_		= nullptr;
    uiToolButton*		minsetbut_;
    uiGenInput*			maxfld_		= nullptr;
    uiToolButton*		maxsetbut_;
};

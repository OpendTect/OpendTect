#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uimainwin.h"

#include "datapack.h"
#include "survinfo.h"
#include "odcomplex.h"

class uiCheckBox;
class uiGenInput;
class uiFuncDispBase;
class uiLabeledSpinBox;
class uiPushButton;
namespace Fourier { class CC; }
template <class T> class Array1D;
template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class Array1DImpl;


mExpClass(uiTools) uiAmplSpectrum : public uiMainWin
{ mODTextTranslationClass(uiAmplSpectrum);
public:
    struct Setup
    {
			Setup( const uiString& t=uiString::emptyString(),
			       bool iscep=false, float nyqst=SI().zStep() )
			    : caption_(t)
			    , nyqvistspspace_(nyqst)
			    , iscepstrum_(iscep)	{}

	mDefSetupMemb(uiString,caption)
	mDefSetupMemb(float,nyqvistspspace)
	mDefSetupMemb(bool,iscepstrum)
    };

				uiAmplSpectrum(uiParent*,
					const uiAmplSpectrum::Setup& =
					uiAmplSpectrum::Setup());
				~uiAmplSpectrum();

    void			setDataPackID(DataPackID,DataPackMgr::MgrID,
					      int version=0);
    void			setData(const float* array,int size);
    void			setData(const Array1D<float>&);
    void			setData(const Array2D<float>&);
    void			setData(const Array3D<float>&);

    void			getSpectrumData(Array1DImpl<float>&,
						bool normalized=false);
    Interval<float>		getPosRange() const { return posrange_; }

protected:

    uiFuncDispBase*		disp_;
    uiGenInput*			rangefld_;
    uiLabeledSpinBox*		stepfld_;
    uiGenInput*			valfld_;
    uiGroup*			dispparamgrp_;
    uiPushButton*		exportfld_;
    uiCheckBox*			normfld_;
    uiCheckBox*			powerdbfld_;

    void			initFFT(int nrsamples);
    bool			compute(const Array3D<float>&);
    void			putDispData(CallBacker*);
    void			valChgd(CallBacker*);

    uiAmplSpectrum::Setup	setup_;

    Array3D<float>*		data_;
    Array1DImpl<float_complex>* timedomain_;
    Array1DImpl<float_complex>* freqdomain_;
    Array1DImpl<float>*		freqdomainsum_;
    Array1DImpl<float>*		specvals_;
    float			maxspecval_;

    Interval<float>		posrange_;

    Fourier::CC*		fft_;
    int				nrtrcs_;

    void			dispRangeChgd(CallBacker*);
    void			exportCB(CallBacker*);
    void			ceptrumCB(CallBacker*);
};

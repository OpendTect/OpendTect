#ifndef uiamplspectrum_h
#define uiamplspectrum_h

/*
________________________________________________________________________
            
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          September 2007
RCS:           $Id: uiamplspectrum.h,v 1.15 2011-11-03 09:22:16 cvsbruno Exp $
______________________________________________________________________
                       
*/   

#include "uidialog.h"
#include "datapack.h"

#include <complex>
typedef std::complex<float> float_complex;

class uiFunctionDisplay;
class uiGenInput;
class uiLabeledSpinBox;
namespace Fourier { class CC; }
template <class T> class Array1D;
template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class Array1DImpl;


mClass uiAmplSpectrum : public uiMainWin
{
public:
    				uiAmplSpectrum(uiParent*,bool wantcp=true);
				~uiAmplSpectrum();

    void			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const float* array,int size);
    void			setData(const Array1D<float>&);
    void			setData(const Array2D<float>&);
    void			setData(const Array3D<float>&);

    void			getSpectrumData(Array1DImpl<float>&);
    Interval<float>		getPosRange() const { return posrange_; }

protected:

    uiFunctionDisplay*		disp_;
    uiGenInput*			rangefld_;
    uiLabeledSpinBox*		stepfld_;
    uiGenInput*			valfld_;
    uiGroup*			dispparamgrp_;

    void			initFFT(int nrsamples);
    bool			compute(const Array3D<float>&);
    void			putDispData();
    void			valChgd(CallBacker*);
    
    Array1DImpl<float_complex>* timedomain_;
    Array1DImpl<float_complex>* freqdomain_;
    Array1DImpl<float_complex>* qfredomain_;
    Array1DImpl<float_complex>* freqdomainsum_;
    Array1DImpl<float>* 	specvals_;
    DataPack::ID		dpid_;
    DataPackMgr::ID		dmid_;

    Interval<float>		posrange_;
    float			nyqvistspspace_;

    Fourier::CC*		fft_;
    int				nrtrcs_;
    bool			wantceptrum_;

    void			dispRangeChgd(CallBacker*);
    void			exportCB(CallBacker*);
    void			ceptrumCB(CallBacker*);
};


#endif

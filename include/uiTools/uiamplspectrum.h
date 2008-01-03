#ifndef uiamplspectrum_h
#define uiamplspectrum_h

/*
________________________________________________________________________
            
CopyRight:     (C) dGB Beheer B.V.
Author:        Satyaki Maitra
Date:          September 2007
RCS:           $Id: uiamplspectrum.h,v 1.2 2008-01-03 12:17:24 cvsnanne Exp $
______________________________________________________________________
                       
*/   

#include "uidialog.h"
#include "datapack.h"

#include <complex>
typedef std::complex<float> float_complex;

class uiCanvas;
class uiHistogramDisplay;
class FFT;

template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class Array1DImpl;

class uiAmplSpectrum : public uiDialog
{
public:
    				uiAmplSpectrum( uiParent* p );
				~uiAmplSpectrum();

    void			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const Array2D<float>&);
    void			setData(const Array3D<float>&);

protected:
    uiCanvas*                   canvas_;
    uiHistogramDisplay*         histogramdisplay_;

    void                        reDraw(CallBacker*);

    void			initFFT(int nrsamples);
    bool			compute(const Array3D<float>&);
    void			draw();
    
    Array1DImpl<float_complex>* timedomain_;
    Array1DImpl<float_complex>* freqdomain_;
    Array1DImpl<float_complex>* freqdomainsum_;

    FFT*			fft_;
    uiRect			bdrect_;
    int				nrtrcs_;
    StepInterval<float>         xrg_;
};


#endif

#ifndef uiamplspectrum_h
#define uiamplspectrum_h

/*
________________________________________________________________________
            
CopyRight:     (C) dGB Beheer B.V.
Author:        Satyaki Maitra
Date:          September 2007
RCS:           $Id: uiamplspectrum.h,v 1.5 2009-03-10 06:33:51 cvssatyaki Exp $
______________________________________________________________________
                       
*/   

#include "uidialog.h"
#include "datapack.h"

#include <complex>
typedef std::complex<float> float_complex;

class uiFunctionDisplay;
class FFT;
template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class Array1DImpl;


mClass uiAmplSpectrum : public uiMainWin
{
public:
    				uiAmplSpectrum(uiParent*);
				~uiAmplSpectrum();

    void			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const Array2D<float>&);
    void			setData(const Array3D<float>&);

protected:

    uiFunctionDisplay*		disp_;

    void			initFFT(int nrsamples);
    bool			compute(const Array3D<float>&);
    void			putDispData();
    
    Array1DImpl<float_complex>* timedomain_;
    Array1DImpl<float_complex>* freqdomain_;
    Array1DImpl<float_complex>* freqdomainsum_;

    FFT*			fft_;
    int				nrtrcs_;
};


#endif

#ifndef uifkspectrum_h
#define uifkspectrum_h

/*
________________________________________________________________________
            
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          September 2007
RCS:           $Id: uifkspectrum.h,v 1.1 2012-04-02 22:45:41 cvsnanne Exp $
______________________________________________________________________
                       
*/   

#include "uidialog.h"
#include "datapack.h"
#include "survinfo.h"

#include <complex>
typedef std::complex<float> float_complex;

namespace Fourier { class CC; }
template <class T> class Array2D;


mClass uiFKSpectrum : public uiMainWin
{
public:
    				uiFKSpectrum(uiParent*);
				~uiFKSpectrum();

    void			setDataPackID(DataPack::ID,DataPackMgr::ID);
    void			setData(const Array2D<float>&);

protected:

    void			initFFT(int,int);
    bool			compute(const Array2D<float>&);
    bool			view(const Array2D<float>&);

    Fourier::CC*		fft_;
    Array2D<float>*		output_;
};


#endif

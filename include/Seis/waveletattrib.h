#ifndef waveletattrib_h
#define waveletattrib_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
 RCS:           $Id: waveletattrib.h,v 1.5 2009-11-19 15:00:17 cvsbruno Exp $
________________________________________________________________________

-*/

#include "commondefs.h"

class FFT;
class HilbertTransform;
class Wavelet;
template <class T> class Array1DImpl;

mClass WaveletAttrib
{
public:
    			WaveletAttrib(const Wavelet&);
			~WaveletAttrib();

    void		getHilbert(Array1DImpl<float>&);
    void		getPhase(Array1DImpl<float>&);

    void		getFrequency(Array1DImpl<float>&,int padfac=1);
    			//frequency array will be rized to padfac*array size )
    void 		getWvltFromFrequency(const Array1DImpl<float>&,
					     Array1DImpl<float>&);
    void		muteZeroFrequency(Array1DImpl<float>&);

protected:

    HilbertTransform*	hilbert_;
    FFT*		fft_;
    int			wvltsz_;
    Array1DImpl<float>* wvltarr_;
};

#endif

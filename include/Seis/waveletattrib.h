#ifndef waveletattrib_h
#define waveletattrib_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
 RCS:           $Id: waveletattrib.h,v 1.1 2009-11-09 06:17:00 cvsnageswara Exp $
________________________________________________________________________

-*/

#include <complex>

class FFT;
class HilbertTransform;
class Wavelet;
template <class T> class Array1DImpl;
typedef std::complex<float> float_complex;

class WaveletAttrib
{
public:
    			WaveletAttrib(const Wavelet&);
			~WaveletAttrib();
    void		getHilbert(Array1DImpl<float>&);
    void		getPhase(Array1DImpl<float>&);
    void		getFrequency(Array1DImpl<float>&,bool ispad=true);
    void		muteZeroFrequency(Array1DImpl<float>&);

protected:

    HilbertTransform*	hilbert_;
    FFT*		fft_;
    int			wvltsz_;
    Array1DImpl<float>* wvltarr_;
};

#endif

#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Nov 2009
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "gendefs.h"
#include "odcomplex.h"
#include "ptrman.h"

class ArrayNDWindow;
class TaskRunnerProvider;
class Wavelet;
template <class T> class Array1DImpl;
template <class T> class Array1D;

mExpClass(Seis) WaveletAttrib
{
public:
			WaveletAttrib(const Wavelet&);
			~WaveletAttrib();

    void		setNewWavelet(const Wavelet&);
    void		getHilbert(Array1DImpl<float>&) const;
    void		getPhase(Array1DImpl<float>&,bool degree=false) const;
    float		getAvgPhase(bool degree=false) const;
    void		getFrequency(Array1DImpl<float>&,int padfac=1);
			//frequency array will be resized to padfac*array size )
    void		getPhaseRotated(float*,float phase) const;
			/*!<\param phase angle in radians */
    void		getCosTapered(float*,float) const;
    bool		getFreqFiltered(float*,float f1,float f2,
					float f3,float f4) const;
			/*!<\param f1-f4: See FFTFiter class */
    void		applyFreqWindow(const ArrayNDWindow&,int padfac,
					Array1DImpl<float>&);
    void		transform(Array1D<float_complex>&,int sz=-1);
    static void		transformBack(const Array1D<float_complex>& fftwvlt,
				      Array1D<float>& wvlt);

    static void		unwrapPhase(int nrsamples,float wrapparam,float* phase);
    static void		muteZeroFrequency(Array1DImpl<float>&);

protected:

    Array1DImpl<float>* wvltarr_;
    int			wvltsz_;
    int			centersample_;
    float		sr_;
    void		getWaveletArrForPhase(Array1DImpl<float>&) const;
};


			//!<\param filtersz: 0=auto
mGlobal(Seis) ConstRefMan<Wavelet> getMatchFilter(const Wavelet& ref,
						  const Wavelet& target,
						  TaskRunnerProvider&,
						  const int* filtersz =0);

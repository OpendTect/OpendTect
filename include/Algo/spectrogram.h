#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "fourier.h"


/*!
\brief Spectrogram calculates the spectrogram of a N-dimensional signal.
The spectrogram is the square of the absolute values of the FourierTransform,
and is the 'traditional' way to view a signal's frequency distribution.
  
  Spectrogram is not reversible.
*/

mExpClass(Algo) Spectrogram 
{
public:
			Spectrogram();
			~Spectrogram();

    bool		init();
    bool		setInputInfo( const ArrayNDInfo& ni);
    const ArrayNDInfo&	getInputInfo() const;

    bool		real2real() const 		{ return true; }
    bool		real2complex() const 		{ return false; }
    bool		complex2real() const 		{ return false; }
    bool		complex2complex() const 	{ return true; }
    bool		biDirectional() const 		{ return false; }
    bool		getDir() const 			{ return true; }
    bool		setDir( bool fwd ) 		{ return fwd; }

    bool		transform(const ArrayND<float>&,ArrayND<float>&);
    bool		transform(const ArrayND<float_complex>&,
			   	  ArrayND<float_complex>& );
protected:

    bool		isPossible(int) const	{ return true; }
    bool		isFast(int sz) const	{ return fft_->isFast(sz); }

    ArrayND<float_complex>*	tempin_;
    ArrayND<float_complex>*	tempout_;
    Fourier::CC*		fft_;
};

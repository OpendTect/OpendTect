#ifndef phase_h
#define phase_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Dec 2013
 RCS:		$Id$
________________________________________________________________________

*/

#include "algomod.h"
#include "arrayndimpl.h"
#include "odcomplex.h"

/*!
\brief Phase calculates the phase distribution of a N-dimensional signal.
The phase is the inverse tangent of the ratio between imaginary and real parts
of the signal

  Phase is not reversible
*/

mExpClass(Algo) Phase
{
public:
			Phase(const Array1DImpl<float_complex>& cfrequencies);
			Phase(const Array1DImpl<float>& timesignal);
			~Phase();

    void		setUnitDeg( bool indegrees ) { indegrees_ = indegrees; }
    bool		calculate();

			//!< Available after execution - in the Bandwidth
    float		getAvgPhase() const		{ return avgphase_; }
    const Array1DImpl<float>& getPhase() const		{ return phase_; }

protected:
    Array1DImpl<float_complex>&	cfreq_;
    Array1DImpl<float>&	phase_;
    float		domfreqidx_;
    float		avgphase_;
    bool		indegrees_;

    void		init();
    bool		extract();
    bool		convert();
    bool		unWrap(float maxdph=0.01777778f);
			/*!<\param maxdph Maximum allowed phase difference
			  between consecutive frequencies, in radians */
};


#endif


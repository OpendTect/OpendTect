#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "gendefs.h"
class SeisTrc;
class TrcKeyZSampling;


/*!\brief will sub-sample in inl and crl, and re-sample in Z

  If there is inl and crl sub-sampling, get() will return null sometimes.
  If Z needs no resampling and no value range is specified, the input trace
  will be returned.

 */

mExpClass(Seis) SeisResampler
{
public:

			SeisResampler(const TrcKeyZSampling&,bool is2d=false,
				      const Interval<float>* valrange=0);
    			//!< valrange will be copied. null == no checks
			SeisResampler(const SeisResampler&);
    virtual		~SeisResampler();
    SeisResampler&	operator =(const SeisResampler&);

    SeisTrc*		get( SeisTrc& t )	{ return doWork(t); }
    const SeisTrc*	get( const SeisTrc& t )	{ return doWork(t); }

    int			nrPassed() const	{ return nrtrcs; }
    void		set2D( bool yn )	{ is3d = !yn; }

protected:

    SeisTrc*		doWork(const SeisTrc&);

    int			nrtrcs;
    float		replval;
    bool		dozsubsel;
    SeisTrc&		worktrc;
    Interval<float>*	valrg;
    TrcKeyZSampling&	cs;
    bool		is3d;

};

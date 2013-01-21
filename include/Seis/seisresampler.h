#ifndef seisresampler_h
#define seisresampler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		20-1-98
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "gendefs.h"
class SeisTrc;
class CubeSampling;
template <class T> class Interval;


/*!\brief will sub-sample in inl and crl, and re-sample in Z

  If there is inl and crl sub-sampling, get() will return null sometimes.
  If Z needs no resampling and no value range is specified, the input trace
  will be returned.

 */

mExpClass(Seis) SeisResampler
{
public:

    			SeisResampler(const CubeSampling&,bool is2d=false,
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
    CubeSampling&	cs;
    bool		is3d;

};


#endif


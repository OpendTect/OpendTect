#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		20-1-98
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "gendefs.h"
class SeisTrc;
class TrcKeyZSampling;
class CubeSubSel;
class LineSubSelSet;
namespace Seis { class RangeSelData; }


/*!\brief will sub-sample in inl and crl, and re-sample in Z

  If there is inl and crl sub-sampling, get() will return null sometimes.
  If Z needs no resampling and no value range is specified, the input trace
  will be returned.

 */

mExpClass(Seis) SeisResampler
{
public:

    typedef float			value_type;
    typedef float			z_type;
    typedef Interval<value_type>	value_rg_type;
    mUseType( Seis,			RangeSelData );

			SeisResampler(const CubeSubSel&,
				      const value_rg_type* rg=0);
			SeisResampler(const LineSubSelSet&,
				      const value_rg_type* rg=0);
			SeisResampler(const RangeSelData&,
				      const value_rg_type* rg=0);
			SeisResampler(const TrcKeyZSampling&,
				      const value_rg_type* rg=0);
			SeisResampler(const SeisResampler&);
    virtual		~SeisResampler();
    SeisResampler&	operator =(const SeisResampler&);
    bool		is2D() const;

    SeisTrc*		get( SeisTrc& t )	{ return doWork(t); }
    const SeisTrc*	get( const SeisTrc& t )	{ return doWork(t); }

    int			nrPassed() const	{ return nrtrcs_; }

protected:

    SeisTrc*		doWork(const SeisTrc&);

    SeisTrc&		worktrc_;
    RangeSelData&	rsd_;
    int			nrtrcs_			= 0;
    value_type		replval_		= (value_type)0;
    bool		dozsubsel_		= false;
    value_rg_type*	valrg_			= nullptr;

private:

    void		init(const value_rg_type*);

};

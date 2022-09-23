#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "ranges.h"
#include "coord.h"
class TrcKeyZSampling;


namespace Seis
{

/*!\brief contains domain-specific data boundary details.

  start, stop end step are for either first key (Inline or Trace Number) or
  second key (Crossline or aux trace number).

 */

mExpClass(Seis) Bounds
{
public:
			Bounds();
    virtual		~Bounds();

    virtual bool	is2D() const			{ return false; }
    int			expectedNrTraces() const;

    virtual int		start(bool first=true) const		= 0;
			//!< Inl, TrcNr (first) or Crl (2nd)
    virtual int		stop(bool first=true) const		= 0;
			//!< Inl, TrcNr (first) or Crl (2nd)
    virtual int		step(bool first=true) const		= 0;
			//!< Inl, TrcNr (first) or Crl (2nd)
    virtual StepInterval<float> getZRange() const		= 0;

    virtual void	getCoordRange(Coord& min,Coord&) const	= 0;

};


mExpClass(Seis) Bounds3D : public Bounds
{
public:

			Bounds3D();
			Bounds3D(const Bounds3D&);
			~Bounds3D();
    bool		is2D() const override		{ return false; }

    int			start(bool first=true) const override;
    int			stop(bool first=true) const override;
    int			step(bool first=true) const override;
    StepInterval<float> getZRange() const override;

    void		getCoordRange(Coord&,Coord&) const override;

    TrcKeyZSampling&	tkzs_;

};


mExpClass(Seis) Bounds2D : public Bounds
{
public:

			Bounds2D();
			~Bounds2D();

    bool		is2D() const override			{ return true; }

    int			start( bool firstrg = true ) const override
			{ return firstrg ? nrrg_.start : mUdf(int); }
    int			stop( bool firstrg = true ) const override
			{ return firstrg ? nrrg_.stop : mUdf(int); }
    int			step( bool firstrg = true ) const override
			{ return firstrg ? nrrg_.step : 1; }
    StepInterval<float> getZRange() const override
			{ return zrg_; }

    void		getCoordRange( Coord& min, Coord& max ) const override
			{ min = mincoord_; max = maxcoord_; }

    StepInterval<float> zrg_;
    StepInterval<int>	nrrg_;
    Coord		mincoord_;
    Coord		maxcoord_;

};

} // namespace Seis

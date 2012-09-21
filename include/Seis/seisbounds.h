#ifndef seisbounds_h
#define seisbounds_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		20-1-98
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "ranges.h"
#include "position.h"
class CubeSampling;


namespace Seis
{

/*!\brief contains domain-specific data boundary details.

  start, stop end step are for either first key (Inline or Trace Number) or
  second key (Crossline or aux trace number).
 
 */

mClass(Seis) Bounds
{
public:

    virtual		~Bounds()			{}

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


mClass(Seis) Bounds3D : public Bounds
{
public:

    			Bounds3D();
    			Bounds3D(const Bounds3D&);
    			~Bounds3D();
    bool		is2D() const			{ return false; }

    int			start(bool first=true) const;
    int			stop(bool first=true) const;
    int			step(bool first=true) const;
    StepInterval<float>	getZRange() const;

    void		getCoordRange(Coord&,Coord&) const;

    CubeSampling&	cs_;

};


mClass(Seis) Bounds2D : public Bounds
{
public:

    			Bounds2D();
    bool		is2D() const			{ return true; }

    int			start( bool firstrg = true ) const
			{ return firstrg ? nrrg_.start : mUdf(int); }
    int			stop( bool firstrg = true ) const
			{ return firstrg ? nrrg_.stop : mUdf(int); }
    int			step( bool firstrg = true ) const
			{ return firstrg ? nrrg_.step : 1; }
    StepInterval<float>	getZRange() const
			{ return zrg_; }

    void		getCoordRange( Coord& min, Coord& max ) const
			{ min = mincoord_; max = maxcoord_; }

    StepInterval<float>	zrg_;
    StepInterval<int>	nrrg_;
    Coord		mincoord_;
    Coord		maxcoord_;

};

} // namespace Seis

#endif


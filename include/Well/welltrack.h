#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellmod.h"
#include "welldahobj.h"
#include "position.h"
#include "welldata.h"

class UnitOfMeasure;

namespace Well
{

/*!
\brief Well track
*/

mExpClass(Well) Track : public DahObj
{
public:

			Track( const char* nm=0 )
			: DahObj(nm), zistime_(false) {}
			Track( const Track& t )
			: DahObj("")		{ *this = t; }
    Track&		operator =(const Track&);

    bool		isEmpty() const;
    const Coord3&	pos( int idx ) const	{ return pos_[idx]; }
    float		value( int idx ) const override
			{ return (float) pos_[idx].z; }

    float		getKbElev() const;
    float		td() const
			{ return isEmpty() ? 0 : dah_.last(); }
    int			size() const	{ return pos_.size(); }
    bool		zIsTime() const		{ return zistime_; }
    const Interval<double> zRangeD() const;
    const Interval<float> zRange() const;
			//!< returns (0, 0) for empty track

    int			insertPoint(const Coord3&);
    int			insertPoint(const Coord&,float z);
			//!< a 'good' place will be found
			//!< If inserted at top, z will be used as first dah
			//!< returns position index of the new point
    void		addPoint(const Coord3&,float dah=mUdf(float));
    void		addPoint(const Coord&,float z,float dah=mUdf(float));
			//!< Point must be further down track. No checks.
    void		setPoint(int,const Coord3&);
    void		setPoint(int,const Coord&,float z);
			//!< Will correct all dahs below point
    void		insertAfterIdx(int,const Coord3&);
			//!< Know what you're doing - not used normally
    void		removePoint(int);
			//!< Will correct all dahs below point

    Coord3		getPos(float d_ah) const;
    const TypeSet<Coord3>& getAllPos() const { return pos_; }

    void		getAllTVD(int sz,const float* md,
				  const UnitOfMeasure* in,float* tvd,
				  const UnitOfMeasure* out,
				  Info::DepthType dtype=Info::TVDSS) const;
			// tvd and dah must both have size sz
    Interval<float>	getTVDRange(const Interval<float>& dahrg,
				    const UnitOfMeasure* in,
				    const UnitOfMeasure* out,
				    Info::DepthType dtype=Info::TVDSS)	const;

    float		getDahForTVD(double,float prevdah=mUdf(float)) const;
    float		getDahForTVD(float,float prevdah=mUdf(float)) const;
			//!< Non-unique. previous DAH may be helpful
			//!< Don't use if track is in time
    float		nearestDah(const Coord3&) const;
			// If zIsTime() z must be time

			// If you know what you're doing:
    Coord3		coordAfterIdx(float d_ah,int) const;
			//!< Beware: no bounds check on index.

    bool		insertAtDah(float dah,float zpos) override;
			//!< will interpolate x,y coords

    bool		alwaysDownward() const;
    void		toTime(const Data&);

protected:


    TypeSet<Coord3>	pos_;
    bool		zistime_;

    void		removeAux( int idx ) override
			{ pos_.removeSingle(idx); }

    void		eraseAux() override		{ pos_.erase(); }

public:

    bool		extendIfNecessary(const Interval<float>& dahrg);
			//!< return if changed

};


}; // namespace Well


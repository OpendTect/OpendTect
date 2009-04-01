#ifndef statdirdata_h
#define statdirdata_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Mar 2009
 RCS:           $Id: statdirdata.h,v 1.2 2009-04-01 14:35:39 cvsbert Exp $
________________________________________________________________________

-*/

#include "manobjectset.h"
#include "angles.h"
#include "ranges.h"

namespace Stats
{

mClass SectorPartData
{
public:

    			SectorPartData( float v=0, float p=0.5 )
			    : val_(v), pos_(p)  {}
    bool		operator ==( const SectorPartData& spd ) const
			{ return pos_ == spd.pos_; }

    float		pos_;	//!< 0=center 1=on circle = maximum value
    float		val_;	//!< can be actual angle or another value

};


typedef TypeSet<SectorPartData> SectorData;


/*!\brief A circle of data.
 
  The circle is subdivided in sectors, which are subdivided in sector parts.
  The relative 'pos_' from the sector part can be scaled with usrposrg_ to get
  user positions.

  Angles are implicit: we always expect 360 degrees of data.

 */

mClass DirectionalData : public ManagedObjectSet<SectorData>
{
public:

    mClass Setup
    {
    public:
    			Setup()
			    : usrposrg_(0,1)
    			    , angle0_(0)
    			    , angletype_(Angle::UsrDeg)	{}

	Interval<float>	usrposrg_;
	float		angle0_;
	Angle::Type	angletype_;
    };

    			DirectionalData()
			    : ManagedObjectSet<SectorData>(false)	{}

    SectorPartData&	get( int isect, int ipart )
			{ return (*((*this)[isect]))[ipart]; }
    const SectorPartData& get( int isect, int ipart ) const
			{ return (*((*this)[isect]))[ipart]; }
    inline int		nrSectors() const
			{ return size(); }
    inline int		nrParts( int isect ) const
			{ return ((*this)[isect])->size(); }
    inline float	angle(int isect) const;

    Setup		setup_;

};


inline float DirectionalData::angle( int isect ) const
{
    float fullc; Angle::getFullCircle( setup_.angletype_, fullc );
    const float angstep = fullc / size();
    return setup_.angle0_ + angstep * isect;
}


}; // namespace Stats

#endif

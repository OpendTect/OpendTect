#ifndef statdirdata_h
#define statdirdata_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Mar 2009
 RCS:           $Id: statdirdata.h,v 1.9 2012-07-10 08:05:25 cvskris Exp $
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

    			SectorPartData( float v=0, float p=0.5, int cnt=0 )
			    : val_(v), pos_(p), count_(cnt)  {}
    bool		operator ==( const SectorPartData& spd ) const
			{ return pos_ == spd.pos_; }

    float		pos_;	//!< 0=center 1=on circle = maximum value
    float		val_;	//!< actual angle or a value of interest
    int			count_;	//!< nr data pts contributing (for confidence)

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
			DirectionalData(int nrsectors,int nrparts=0);

    SectorPartData&	get( int isect, int ipart )
			{ return (*((*this)[isect]))[ipart]; }
    const SectorPartData& get( int isect, int ipart ) const
			{ return (*((*this)[isect]))[ipart]; }
    inline int		nrSectors() const
			{ return size(); }
    inline int		nrParts( int isect ) const
			{ return ((*this)[isect])->size(); }
    inline float	angle(int isect,int bound=0) const;
    inline float	angle(int isect,Angle::Type,int bound=0) const;
    			//!< bound: -1=start, 1=stop, 0=center
    int			sector(float ang) const;
    int			sector(float ang,Angle::Type) const;

    Setup		setup_;

};


inline float DirectionalData::angle( int isect, int bound ) const
{
    float fullc; Angle::getFullCircle( setup_.angletype_, fullc );
    const float angstep = fullc / size();
    const float centerang = setup_.angle0_ + angstep * isect;
    return centerang + bound * angstep * .5;
}


inline float DirectionalData::angle( int isect, Angle::Type t, int bound ) const
{
    float ang = angle( isect, bound );
    return Angle::convert( setup_.angletype_, ang, t );
}


inline int DirectionalData::sector( float ang, Angle::Type t ) const
{
    return sector( Angle::convert(t,ang,setup_.angletype_) );
}


inline int DirectionalData::sector( float ang ) const
{
    ang -= setup_.angle0_;
    const float usrang = Angle::convert(setup_.angletype_,ang,Angle::UsrDeg);
    float fsect = size() * (ang / 360);
    int sect = mNINT32(fsect);
    if ( sect >= size() ) sect = 0;
    return sect;
}


inline DirectionalData::DirectionalData( int nrsect, int nrparts )
    : ManagedObjectSet<SectorData>(false)
{
    for ( int isect=0; isect<nrsect; isect++ )
    {
	SectorData* sd = new SectorData;
	*this += sd;
	for ( int ipart=0; ipart<nrparts; ipart++ )
	    *sd += SectorPartData( 0, (ipart + .5) / nrparts, 0 );
    }
}


}; // namespace Stats

#endif

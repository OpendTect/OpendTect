#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "manobjectset.h"
#include "angles.h"
#include "ranges.h"
#include "typeset.h"

namespace Stats
{

/*!
\brief Part of a data sector.
*/

mExpClass(Algo) SectorPartData
{
public:

			SectorPartData(float v=0,float p=0.5,int cnt=0);
			SectorPartData(const SectorPartData&);
			~SectorPartData();

    bool		operator ==(const SectorPartData&) const;

    float		val_;	//!< actual angle or a value of interest
    float		pos_;	//!< 0=center 1=on circle = maximum value
    int			count_;	//!< nr data pts contributing (for confidence)

};


using SectorData = TypeSet<SectorPartData>;


/*!
\brief A circle of data.

  The circle is subdivided in sectors, which are subdivided in sector parts.
  The relative 'pos_' from the sector part can be scaled with usrposrg_ to get
  user positions.

  Angles are implicit: we always expect 360 degrees of data.
*/

mExpClass(Algo) DirectionalData : public ManagedObjectSet<SectorData>
{
public:

    mExpClass(Algo) Setup
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

			DirectionalData(int nrsectors=0,int nrparts=0);
			DirectionalData(const DirectionalData&);
			~DirectionalData();

    DirectionalData&	operator=(const DirectionalData&);

    void		init(int nrsectors,int nrparts);

    SectorPartData&	getPartData( int isect, int ipart )
			{ return (*((*this)[isect]))[ipart]; }
    const SectorPartData& getPartData( int isect, int ipart ) const
			{ return (*((*this)[isect]))[ipart]; }
    inline int		nrSectors() const
			{ return size(); }
    inline int		nrParts( int isect ) const
			{ return ((*this)[isect])->size(); }

    float		angle(int isect,int bound=0) const;
    float		angle(int isect,Angle::Type,int bound=0) const;
			//!< bound: -1=start, 1=stop, 0=center
    int			sector(float ang) const;
    int			sector(float ang,Angle::Type) const;
    int			part(int isect,float pos) const;

    Setup		setup_;

};

} // namespace Stats

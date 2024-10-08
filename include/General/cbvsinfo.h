#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "basiccompinfo.h"
#include "posauxinfo.h"
#include "posidxpair2coord.h"
#include "posinfo.h"
#include "samplingdata.h"

class TrcKeyZSampling;


/*!\brief Data available in CBVS format header and trailer.

Some info per position is explicitly stored, other is implicit or not present.
If the SurvGeom has full rectangularity, cubedata can be ignored.

*/

mExpClass(General) CBVSInfo
{
public:

				CBVSInfo();
				CBVSInfo(const CBVSInfo&);
				~CBVSInfo();

    CBVSInfo&			operator =(const CBVSInfo&);

    int				estimatedNrTraces() const;

    mStruct(General) SurvGeom
    {
				SurvGeom();
				SurvGeom(const SurvGeom&);
				~SurvGeom();

	SurvGeom&		operator=(const SurvGeom&);

	bool			fullyrectandreg_ = false;
	BinID			start_;
	BinID			stop_;
	BinID			step_;	//!< step can be < 0
	Pos::IdxPair2Coord	b2c_;
	PosInfo::SortedCubeData cubedata_;

	void			merge(const SurvGeom&);
	void			reCalcBounds();

	int			excludes(const BinID&) const;
	inline bool		includes( const BinID& bid ) const
				{ return !excludes(bid); }
	bool			includesInline(int) const;
	void			clean();

	bool			moveToNextPos(BinID&) const;
	bool			moveToNextInline(BinID&) const;

	mDeprecated("Use fullyrectandreg_")
	bool&			fullyrectandreg;
	mDeprecated("Use start_")
	BinID&			start;
	mDeprecated("Use stop_")
	BinID&			stop;
	mDeprecated("Use step_")
	BinID&			step;	//!< step can be < 0
	mDeprecated("Use b2c_")
	Pos::IdxPair2Coord&	b2c;
	mDeprecated("Use cubedata_")
	PosInfo::SortedCubeData& cubedata;

protected:

	void			toIrreg();
	void			mergeIrreg(const SurvGeom&);
	int			outOfRange(const BinID&) const;

    };

    int				seqnr_ = 0;
    int				nrtrcsperposn_ = 1;

    PosAuxInfoSelection		auxinfosel_;
    ObjectSet<BasicComponentInfo> compinfo_;
    SamplingData<float>		sd_;
    int				nrsamples_;
    SurvGeom			geom_;

    BufferString		stdtext_;
    BufferString		usertext_;

    bool			contributesTo(const TrcKeyZSampling&) const;
    void			clean()
				{ deepErase(compinfo_); geom_.clean();
				  usertext_ = ""; }

};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "generalmod.h"
#include "posauxinfo.h"
#include "posidxpair2coord.h"
#include "basiccompinfo.h"
#include "samplingdata.h"
#include "posinfo.h"
#include "scaler.h"
class TrcKeyZSampling;


/*!\brief Data available in CBVS format header and trailer.

Some info per position is explicitly stored, other is implicit or not present.
If the SurvGeom has full rectangularity, cubedata can be ignored.

*/

mExpClass(General) CBVSInfo
{
public:

				CBVSInfo()			{}
				~CBVSInfo()	{ deepErase(compinfo_); }
				CBVSInfo( const CBVSInfo& oth )
				{ *this = oth; }
    CBVSInfo&			operator =(const CBVSInfo&);

    int				estimatedNrTraces() const;

    mStruct(General) SurvGeom
    {
				SurvGeom()			{}

	bool			fullyrectandreg = false;
	BinID			start, stop, step;	//!< step can be < 0
	Pos::IdxPair2Coord	b2c;
	PosInfo::SortedCubeData	cubedata;

	void			merge(const SurvGeom&);
	void			reCalcBounds();

	int			excludes(const BinID&) const;
	inline bool		includes( const BinID& bid ) const
				{ return !excludes(bid); }
	bool			includesInline(int) const;
	void			clean()
				{ fullyrectandreg = false; deepErase(cubedata);}

	bool			moveToNextPos(BinID&) const;
	bool			moveToNextInline(BinID&) const;

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

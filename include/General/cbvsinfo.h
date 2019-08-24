#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
________________________________________________________________________

-*/

#include "generalmod.h"
#include "basiccompinfo.h"
#include "cubedata.h"
#include "posauxinfo.h"
#include "posidxpair2coord.h"
#include "samplingdata.h"
#include "scaler.h"
class TrcKeyZSampling;


/*!\brief Data available in CBVS format header and trailer.

Some info per position is explicitly stored, other is implicit or not present.
If the SurvGeom has full rectangularity, cubedata can be ignored.

*/

mExpClass(General) CBVSInfo
{
public:

				CBVSInfo()
				: seqnr_(0), nrtrcsperposn_(1)	{}
				~CBVSInfo()	{ deepErase(compinfo_); }
				CBVSInfo( const CBVSInfo& ci )
				{ *this = ci; }
    CBVSInfo&			operator =(const CBVSInfo&);

    mStruct(General) SurvGeom
    {
				SurvGeom()
				: fullyrectandreg(false)	{}

	bool			fullyrectandreg;
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

    int				seqnr_;
    int				nrtrcsperposn_;

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

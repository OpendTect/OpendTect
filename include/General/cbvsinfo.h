#ifndef cbvsinfo_h
#define cbvsinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsinfo.h,v 1.18 2005-01-05 15:06:57 bert Exp $
________________________________________________________________________

-*/

#include "posauxinfo.h"
#include "binid2coord.h"
#include "basiccompinfo.h"
#include "samplingdata.h"
#include "segposinfo.h"
#include "scaler.h"
class CubeSampling;


/*!\brief Data available in CBVS format header and trailer.

Some info per position is explicitly stored, other is implicit or not present.
If the SurvGeom has full rectangularity, cubedata can be ignored.

*/

class CBVSInfo
{
public:

				CBVSInfo()
				: seqnr(0), nrtrcsperposn(1)	{}
				~CBVSInfo()	{ deepErase(compinfo); }
				CBVSInfo( const CBVSInfo& ci )
				{ *this = ci; }
    CBVSInfo&			operator =(const CBVSInfo&);

    struct SurvGeom
    {
				SurvGeom()
				: fullyrectandreg(false)	{}

	bool			fullyrectandreg;
	BinID			start, stop, step;
				//!< If step < 0, the order is reversed in
				//!< the file
	BinID2Coord		b2c;
	PosInfo::CubeData	cubedata;
				//!< For write, cubedata is ignored in favor
				//!< of actually written, which is put
				//!< in trailer.

	void			merge(const SurvGeom&);
	PosInfo::InlData*	getInfoFor( int inl )
				{ return gtInfFor(inl); }
				//!< returns 0 in case of regular
	const PosInfo::InlData*	getInfoFor( int inl ) const
				{ return gtInfFor(inl); }
	void			reCalcBounds();

	int			excludes(const BinID&) const;
	inline bool		includes( const BinID& bid ) const
				{ return !excludes(bid); }
	bool			includesInline(int) const;
	bool			toNextInline(BinID&) const;
	bool			toNextBinID(BinID&) const;
	void			clean()
	    			{ fullyrectandreg = false; deepErase(cubedata);}

    protected:

	void			toIrreg();
	void			mergeIrreg(const SurvGeom&);
	int			outOfRange(const BinID&) const;
	int			getInfIdx(const BinID&,int&) const;
	int			getInfoIdxFor(int) const;
	PosInfo::InlData*	gtInfFor(int) const;

    };

    int				seqnr;
    int				nrtrcsperposn;

    PosAuxInfoSelection		auxinfosel;
    ObjectSet<BasicComponentInfo> compinfo;
    SamplingData<float>		sd;
    int				nrsamples;
    SurvGeom			geom;

    BufferString		stdtext;
    BufferString		usertext;

    bool			contributesTo(const CubeSampling&) const;
    void			clean()
				{ deepErase(compinfo); geom.clean();
				  usertext = ""; }

};


#endif

#ifndef cbvsinfo_h
#define cbvsinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsinfo.h,v 1.1 2001-03-19 10:17:57 bert Exp $
________________________________________________________________________

-*/

#include <binid2coord.h>
#include <basiccompinfo.h>
#include <sets.h>


/*!\brief Data available in CBVS format header and trailer.

Some info per position is explicitly stored, other is implicit or not present. If
the SurvGeom has full rectangularity, inldata can be ignored.

*/

class CBVSInfo
{
public:

				CBVSInfo()
				: seqnr(0), nrtrcsperposn(1)	{}
				~CBVSInfo()	{ deepErase(compinfo); }

    struct SurvGeom
    {
				SurvGeom()
				: fullyrectangular(false)	{}
				~SurvGeom()	{ deepErase(inldata); }

	struct InlineInfo
	{
	    typedef StepInterval<int> Segment;		//!< xline numbers

				InlineInfo( int i=0 )
				: inl(i)		{}

	    int			inl;
	    TypeSet<Segment>	segments;
	};

	bool			fullyrectangular;
	BinID			start, stop, step;
	BinID2Coord		b2c;
	ObjectSet<InlineInfo>	inldata;
				//!< For write, inldata is ignored in favor
				//!< of actually written, which is put
				//!< in trailer.

    };

    struct ExplicitInfo
    {
		ExplicitInfo()
		: startpos(false), coord(false)
		, offset(false), pick(false), refpos(false)	{}

	bool	startpos;
	bool	coord;
	bool	offset;
	bool	pick;
	bool	refpos;
    };

    struct ExplicitData
    {
		ExplicitData()
		: startpos(0), coord(0,0)
		, offset(0), pick(mUndefValue), refpos(mUndefValue)	{}

	float	startpos;
	Coord	coord;
	float	offset;
	float	pick;
	float	refpos;
    };

    ExplicitInfo		explinfo;
    ObjectSet<BasicComponentInfo> compinfo;

    int				nrtrcsperposn;
    SurvGeom			geom;

    int				seqnr;
    BufferString		usertext;

    void			clean()
				{ compinfo.erase(); geom.inldata.erase();
				  usertext = ""; }

};


#endif

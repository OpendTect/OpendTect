#ifndef cbvsinfo_h
#define cbvsinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsinfo.h,v 1.6 2001-04-18 14:45:36 bert Exp $
________________________________________________________________________

-*/

#include <binid2coord.h>
#include <basiccompinfo.h>
#include <scaler.h>
#include <sets.h>


/*!\brief CBVS component info is Basic + linear scaler. */

class CBVSComponentInfo : public BasicComponentInfo
{
public:
			CBVSComponentInfo( const char* nm=0 )
			: BasicComponentInfo(nm)
			, scaler(0)			{}
			CBVSComponentInfo(const CBVSComponentInfo& cci )
			: BasicComponentInfo(cci)
			, scaler(cci.scaler?cci.scaler->duplicate():0)	{}
    virtual		~CBVSComponentInfo()		{ delete scaler; }
    CBVSComponentInfo&	operator =( const BasicComponentInfo& cci )
			{
			    if ( this == &cci ) return *this;
			    BasicComponentInfo::operator =( cci );
			    const CBVSComponentInfo* cbcci =
				dynamic_cast<const CBVSComponentInfo*>(&cci);
			    if ( cbcci )
			    {
				delete scaler;
				scaler = cbcci->scaler
				       ? cbcci->scaler->duplicate() : 0;
			    }
			    return *this;
			}
    bool		operator ==( const CBVSComponentInfo& cci ) const
			{ return BasicComponentInfo::operator ==(cci)
			      && ( (!scaler && !cci.scaler)
				|| (scaler && cci.scaler
				  && *scaler == *cci.scaler) );
			}

    LinScaler*		scaler;

};


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
				CBVSInfo(const CBVSInfo&);
    inline CBVSInfo&		operator =(const CBVSInfo&);

    struct SurvGeom
    {
				SurvGeom()
				: fullyrectandreg(false)	{}
				~SurvGeom()	{ deepErase(inldata); }
				SurvGeom( const SurvGeom& sg )
				{ *this = sg; }
	inline SurvGeom&	operator =(const SurvGeom&);

	struct InlineInfo
	{
	    typedef StepInterval<int> Segment;		//!< xline numbers

				InlineInfo( int i=0 )
				: inl(i)		{}

	    int			inl;
	    TypeSet<Segment>	segments;
	};

	bool			fullyrectandreg;
	BinID			start, stop, step;
				//!< If step < 0, the order is reversed in
				//!< the file
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
		: binid(0,0), startpos(0), coord(0,0)
		, offset(0), pick(mUndefValue), refpos(mUndefValue)	{}

	BinID	binid;
		//!< For write: must be filled if SurvGeom::fullyrectandreg
		//!< is false. For read will be filled always.
	float	startpos;
	Coord	coord;
	float	offset;
	float	pick;
	float	refpos;
    };


    int				seqnr;
    int				nrtrcsperposn;

    ExplicitInfo		explinfo;
    ObjectSet<CBVSComponentInfo> compinfo;
    SurvGeom			geom;

    BufferString		stdtext;
    BufferString		usertext;

    void			clean()
				{ compinfo.erase(); geom.inldata.erase();
				  usertext = ""; }

};


inline CBVSInfo::SurvGeom& CBVSInfo::SurvGeom::operator =(
	const CBVSInfo::SurvGeom::SurvGeom& sg )
{
    fullyrectandreg = sg.fullyrectandreg;
    start = sg.start;
    stop = sg.stop;
    step = sg.step;
    b2c = sg.b2c;

    for ( int idx=0; idx<sg.inldata.size(); idx++ )
	inldata += new CBVSInfo::SurvGeom::InlineInfo( *sg.inldata[idx] );

    return *this;
}


inline CBVSInfo& CBVSInfo::operator =( const CBVSInfo& ci )
{
    seqnr = ci.seqnr;
    nrtrcsperposn = ci.nrtrcsperposn;
    explinfo = ci.explinfo;
    geom = ci.geom;
    stdtext = ci.stdtext;
    usertext = ci.usertext;

    for ( int idx=0; idx<ci.compinfo.size(); idx++ )
	compinfo += new CBVSComponentInfo( *ci.compinfo[idx] );

    return *this;
}


#endif

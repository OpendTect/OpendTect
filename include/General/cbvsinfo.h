#ifndef cbvsinfo_h
#define cbvsinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format header
 RCS:		$Id: cbvsinfo.h,v 1.8 2001-05-02 13:50:04 windev Exp $
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
			, scaler(cci.scaler
			  ? mPolyRetDownCast(LinScaler*,cci.scaler->duplicate())
			  : 0) 
			{}

    virtual		~CBVSComponentInfo()
			{ delete scaler; scaler = 0; }
    CBVSComponentInfo&	operator =( const BasicComponentInfo& cci )
			{
			    if ( this == &cci ) return *this;
			    BasicComponentInfo::operator =( cci );
			    const CBVSComponentInfo* cbcci =
				dynamic_cast<const CBVSComponentInfo*>(&cci);
			    if ( cbcci )
			    {
				delete scaler;
				scaler = cbcci->scaler ? mPolyRetDownCast(
				    LinScaler*, cbcci->scaler->duplicate()) : 0;
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
				CBVSInfo( const CBVSInfo& ci )
				{ *this = ci; }
    CBVSInfo&			operator =(const CBVSInfo&);

    struct SurvGeom
    {
				SurvGeom()
				: fullyrectandreg(false)	{}
				~SurvGeom()	{ deepErase(inldata); }
				SurvGeom( const SurvGeom& sg )
				{ *this = sg; }
	SurvGeom&		operator =(const SurvGeom&);

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

	void			merge(const SurvGeom&);
	InlineInfo*		getInfoFor(int inl);
				//!< returns 0 in case of regular
	void			reCalcBounds();

    protected:

	void			toIrreg();
	void			mergeIrreg(const SurvGeom&);

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

/*
    The size of the following data structures must be known in
    CBVSReader::getExplicits. When changing the type of one of these attributes,
    make sure to also change the corresponding parts in the __msvc__ part of 
    CBVSReader::getExplicits.
*/
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


#endif

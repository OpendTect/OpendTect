#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "ranges.h"
#include "typeset.h"
#include "trckey.h"

namespace OD
{
    namespace JSON
    {
	class Object;
    };
};

class StringPairSet;

typedef TypeSet<TrcKey> TrcKeyPath;

/*!
\brief Horizontal sampling (inline and crossline range and steps).
*/

mExpClass(Basic) TrcKeySampling
{
public:
			TrcKeySampling();
			TrcKeySampling(const TrcKeySampling&);
			TrcKeySampling(const Pos::GeomID&);
    //Legacy. Will be removed
    explicit		TrcKeySampling(bool settoSI);
			~TrcKeySampling();

    bool		is2D() const	{ return ::is2D(survid_); }
    Pos::GeomID		getGeomID() const;
    void		setGeomID(const Pos::GeomID&);

    TrcKeySampling&	set(const Interval<int>& linerg,
			    const Interval<int>& trcnrrg);
			    //!< steps copied if available
    TrcKeySampling&	set(const Pos::GeomID&,const Interval<int>& trcnrrg);
    void		get(Interval<int>& linerg,Interval<int>& trcnrrg) const;
			    //!< steps filled if available
    TrcKeySampling	getLineChunk(int totalchunks,int chunknr) const;
			//!< totalchunks > 0, 0 <= chunknr < totalchunks

    StepInterval<int>	lineRange() const;
    StepInterval<int>	trcRange() const;
    float		lineDistance() const;
			/*!< real world distance between 2 lines incremented by
			     one times the step_ */
    float		trcDistance() const;
			/*!< real world distance between 2 traces incremented by
			     one times the step_ */
    void		setLineRange(const Interval<int>&);
    void		setTrcRange(const Interval<int>&);

    bool		includes(const TrcKeySampling&,
				 bool ignoresteps=false) const;
    bool		includes(const TrcKey&) const;
    bool		includes(const TrcKey&,bool ignoresteps) const;
    bool		lineOK(Pos::LineID) const;
    bool		trcOK(Pos::TraceID) const;

    bool		lineOK(Pos::LineID,bool ignoresteps) const;
    bool		trcOK(Pos::TraceID,bool ignoresteps) const;

    void		include(const TrcKey&);
    void		includeLine(Pos::LineID);
    void		includeTrc(Pos::TraceID);
    void		include(const TrcKeySampling&, bool ignoresteps=false );
    bool		isDefined() const;
    void		limitTo(const TrcKeySampling&,bool ignoresteps=false);
    void		limitToWithUdf(const TrcKeySampling&);
			    /*!< handles undef values +returns reference HS
				 nearest limit if HS's do not intersect */
    void		shrinkTo(const TrcKeySampling& innertks);
    void		growTo(const TrcKeySampling& outertks);
    void		expand(int nrlines,int nrtrcs);

    int			lineIdxFromGlobal(od_int64) const;
    int			trcIdxFromGlobal(od_int64) const;
    int			lineIdx(Pos::LineID) const;
    int			trcIdx(Pos::TraceID) const;
    inline Pos::LineID	lineID(int) const;
    inline Pos::TraceID	traceID(int) const;

    od_int64		globalIdx(const TrcKey&) const;
    od_int64		globalIdx(const BinID&) const;
    BinID		atIndex(int i0,int i1) const;
    BinID		atIndex(od_int64 globalidx) const;
    TrcKey		trcKeyAt(int i0,int i1) const;
    TrcKey		trcKeyAt(od_int64 globalidx) const;
    TrcKey		toTrcKey(const Coord&,float* distance=nullptr) const;
    Coord		toCoord(const BinID&) const;
    TrcKey		center() const;
    int			nrLines() const;
    int			nrTrcs() const;
    od_int64		totalNr() const;
    bool		isEmpty() const;
    void		neighbors(od_int64 globalidx,TypeSet<od_int64>&) const;
    void		neighbors(const TrcKey&,TypeSet<TrcKey>&) const;

    void		init(bool settoSI=true);
			//!< Sets to survey values or mUdf(int) (but step 1)
    bool		init(const Pos::GeomID&);
    void		init(const TrcKey&);

    void		set2DDef();
			    //!< Sets ranges to 0-maxint
    void		normalize();
			    //!< Makes sure start_<stop_ and steps are non-zero
    mDeprecated("Use normalize()")
    void		normalise();
    void		getRandomSet(int nr,TypeSet<TrcKey>&) const;

    bool		overlaps(const TrcKeySampling&,
				 bool ignoresteps=false) const;
    bool		getInterSection(const TrcKeySampling&,
					TrcKeySampling&) const;
			    //!< Returns false if intersection is empty

    BinID		getNearest(const BinID&) const;
    TrcKey		getNearest(const TrcKey&) const;
			    /*!< step_-snap and outside -> edge.
				Assumes inldist == crldist */
    void		snapToSurvey();
			    /*!< Checks if it is on valid bids. If not, it will
				 expand until it is */

    bool		operator==(const TrcKeySampling&) const;
    bool		operator!=(const TrcKeySampling&) const;
    TrcKeySampling&	operator=(const TrcKeySampling&);

    bool		usePar(const IOPar&);	//!< Keys as in keystrs.h
    void		fillPar(IOPar&) const;	//!< Keys as in keystrs.h
    static void		removeInfo(IOPar&);
    void		fillJSON(OD::JSON::Object&) const;
    bool		useJSON(const OD::JSON::Object&);
    void		toString(BufferString&) const; //!< Nice text for info
    void		fillInfoSet(StringPairSet&) const;

    OD::GeomSystem	survid_;
    BinID		start_;
    BinID		stop_;
    BinID		step_;

    BinID		corner(int idx) const;
			/* idx==0: start_.inl,start_.crl
			   idx==1: stop_.inl,start_.crl
			   idx==2: start_.inl,stop_.crl
			   idx==4: stop_.inl,stop_.crl
			*/

    StepInterval<int>	inlRange() const	{ return lineRange(); }
    StepInterval<int>	crlRange() const	{ return trcRange(); }
    void		setInlRange(const Interval<int>& rg) {setLineRange(rg);}
    void		setCrlRange(const Interval<int>& rg) {setTrcRange(rg);}

    int			nrInl() const { return nrLines(); }
    int			nrCrl() const { return nrTrcs(); }

    int			inlIdx( Pos::LineID lid ) const {return lineIdx(lid);}
    int			crlIdx( Pos::TraceID tid ) const { return trcIdx(tid); }
    inline void		include( const BinID& bid )
			{ includeLine(bid.inl()); includeTrc(bid.crl()); }
    void		includeInl( int inl ) { includeLine(inl); }
    void		includeCrl( int crl ) { includeTrc(crl); }
    inline bool		includes( const BinID& bid ) const
			{ return lineOK(bid.inl()) && trcOK(bid.crl()); }
    inline bool		inlOK( int inl ) const { return lineOK(inl); }
    inline bool		crlOK( int crl ) const { return trcOK(crl); }

    inline bool		includes(const BinID& bid, bool ignoresteps) const
			{ return lineOK(bid.inl(), ignoresteps ) &&
				 trcOK(bid.crl(), ignoresteps ); }

    mDeprecated("Use start_ instead") BinID&		start;
    mDeprecated("Use stop_ instead")  BinID&		stop;
    mDeprecated("Use step_ instead")  BinID&		step;

};


mExpClass(Basic) TrcKeySamplingSet : public TypeSet<TrcKeySampling>
{
public:

    void			isOK() const;
    void			add(Pos::GeomID);
    bool			isPresent(Pos::GeomID);
};



/*!
\brief Finds next BinID in TrcKeySampling; initializes to first position.
*/

mExpClass(Basic) TrcKeySamplingIterator
{
public:
		TrcKeySamplingIterator();
		TrcKeySamplingIterator(const TrcKeySampling&);
		~TrcKeySamplingIterator();

    void	setSampling(const TrcKeySampling&);

    void	reset();
    void	setNextPos(const TrcKey& trk) { curpos_ = tks_.globalIdx(trk); }
    bool	next(TrcKey&) const;
    bool	next(BinID&) const;

    od_int64	curIdx() const		{ return curpos_; }
    TrcKey	curTrcKey() const	{ return tks_.trcKeyAt( curIdx());}

protected:

    TrcKeySampling			tks_;
    od_int64				totalnr_;
    mutable Threads::Atomic<od_int64>	curpos_;
};




typedef TrcKeySampling HorSampling;
typedef TrcKeySamplingIterator	HorSamplingIterator;


inline int TrcKeySampling::lineIdx( Pos::LineID line ) const
{
    return step_.lineNr()
	? (line-start_.lineNr()) / step_.lineNr()
	: (line==start_.lineNr() ? 0 : -1);
}


inline int TrcKeySampling::trcIdx( Pos::TraceID trcid ) const
{
    return step_.trcNr()
	? (trcid-start_.trcNr()) / step_.trcNr()
	: (trcid==start_.trcNr() ? 0 : -1);
}


inline Pos::LineID TrcKeySampling::lineID( int lidx ) const
{
    return start_.lineNr() + step_.lineNr() * lidx;
}


inline Pos::TraceID TrcKeySampling::traceID( int tidx ) const
{
    return start_.trcNr() + step_.trcNr() * tidx;
}

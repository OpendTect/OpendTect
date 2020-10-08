#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "ranges.h"
#include "typeset.h"
#include "trckey.h"
#include "uistring.h"
namespace Survey { class HorSubSel; class Geometry; }
class HorSampling;
class LineHorSubSel;
class CubeHorSubSel;
class TrcKeyZSampling;

namespace OD { namespace JSON {	class Object; }; };

/*!\brief Horizontal sampling (inline and crossline range and steps).  */

mExpClass(Basic) TrcKeySampling
{ mODTextTranslationClass(TrcKeySampling)
public:

    mUseType( OD,	GeomSystem );
    mUseType( Pos,	GeomID );
    mUseType( Survey,	HorSubSel );
    mUseType( TrcKey,	linenr_type );
    mUseType( TrcKey,	trcnr_type );
    mUseType( Survey,	Geometry );
    typedef Pos::Distance_Type	dist_type;

			TrcKeySampling(OD::SurvLimitType slt=OD::FullSurvey);
			TrcKeySampling(GeomID);
			TrcKeySampling(const HorSubSel&);
			TrcKeySampling(const LineHorSubSel&);
			TrcKeySampling(const CubeHorSubSel&);
			TrcKeySampling(const HorSampling&);
			TrcKeySampling(const Geometry&);
			TrcKeySampling(const TrcKeySampling&);
			TrcKeySampling(const TrcKeyZSampling&);
			TrcKeySampling(const TrcKey&);
			TrcKeySampling(bool inittosi,
				       OD::SurvLimitType slt=OD::FullSurvey);

    GeomSystem		geomSystem() const	{ return geomsystem_; }
    bool		is2D() const		{ return ::is2D(geomsystem_); }
    bool		is3D() const		{ return ::is3D(geomsystem_); }
    void		setIs2D()		{ ::set2D( geomsystem_ ); }
    void		setIs3D()		{ ::set3D( geomsystem_ ); }
    void		setGeomID(GeomID);
    GeomID		getGeomID() const;
    void		setIs2D( bool yn )	{ yn ? setIs2D() : setIs3D(); }
    void		setGeomSystem( GeomSystem gs )
						{ geomsystem_ = gs; }

    TrcKeySampling&	set(const Interval<int>& linerg,
			    const Interval<int>& trcnrrg);
				//!< steps copied if available

    void		get(Interval<int>& linerg,Interval<int>& trcnrrg) const;
			    //!< steps filled if available
    TrcKeySampling	getLineChunk(int totalchunks,int chunknr) const;
			//!< totalchunks > 0, 0 <= chunknr < totalchunks

    StepInterval<int>	lineRange() const;
    StepInterval<int>	trcRange() const;
    dist_type		lineDistance() const;
			/*!< real world distance between 2 lines incremented by
			     one times the step_ */
    dist_type		trcDistance() const;
			/*!< real world distance between 2 traces incremented by
			     one times the step_ */
    void		setLineRange(const Interval<int>&);
    void		setTrcRange(const Interval<int>&);

    bool		includes(const TrcKeySampling&,
				 bool ignoresteps=false) const;
    bool		includes(const TrcKey&,bool ignoresteps=false) const;
    bool		lineOK(linenr_type,bool ignoresteps=false) const;
    bool		trcOK(trcnr_type,bool ignoresteps=false) const;

    void		include(const TrcKey&);
    void		includeLine(linenr_type);
    void		includeTrc(trcnr_type);
    void		include(const TrcKeySampling&, bool ignoresteps=false );
    bool		isDefined() const;
    void		limitTo(const TrcKeySampling&,bool ignoresteps=false);
    void		limitToWithUdf(const TrcKeySampling&);
			    /*!< handles undef values +returns reference HS
				 nearest limit if HS's do not intersect */
    void		shrinkTo(const TrcKeySampling& innertks);
    void		growTo(const TrcKeySampling& outertks);
    void		expand(int nrlines,int nrtrcs);

    inline int		lineIdx(linenr_type) const;
    inline int		trcIdx(trcnr_type) const;
    inline linenr_type	lineID(int) const;
    inline trcnr_type	traceID(int) const;

    od_int64		globalIdx(const TrcKey&) const;
    od_int64		globalIdx(const BinID&) const;
    BinID		atIndex(int i0,int i1) const;
    BinID		atIndex(od_int64 globalidx) const;
    TrcKey		trcKeyAt(int i0,int i1) const;
    TrcKey		trcKeyAt(od_int64 globalidx) const;
    TrcKey		toTrcKey(const Coord&,dist_type* distance=nullptr) const;
    Coord		toCoord(const BinID&) const;
    TrcKey		center() const;
    int			nrLines() const;
    int			nrTrcs() const;
    od_int64		totalNr() const;
    bool		isEmpty() const;
    void		neighbors(od_int64 globalidx,TypeSet<od_int64>&) const;
    void		neighbors(const TrcKey&,TypeSet<TrcKey>&) const;
    bool		toNext(BinID&) const;

    void		init(bool settoSI=true,
			     OD::SurvLimitType slt=OD::FullSurvey);
    void		setTo(GeomID);
    void		setTo(const Geometry&);

    void		set2DDef();
			    //!< Sets ranges to 0-maxint
    void		normalise();
			    //!< Makes sure start_<stop_ and steps are non-zero
    void		getRandomSet(int nr,TypeSet<TrcKey>&) const;

    bool		overlaps(const TrcKeySampling&,
				 bool ignoresteps=false) const;
    bool		getIntersection(const TrcKeySampling&,
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
    bool		useJSON(const OD::JSON::Object&);
    void		fillPar(IOPar&) const;	//!< Keys as in keystrs.h
    void		fillJSON(OD::JSON::Object&) const;
    static void		removeInfo(IOPar&);
    void		toString(uiPhrase&) const; //!< Nice text for info

    BinID		start_;
    BinID		stop_;
    BinID		step_;

    StepInterval<int>	inlRange() const	{ return lineRange(); }
    StepInterval<int>	crlRange() const	{ return trcRange(); }
    void		setInlRange(const Interval<int>& rg) {setLineRange(rg);}
    void		setCrlRange(const Interval<int>& rg) {setTrcRange(rg);}

    int			nrInl() const { return nrLines(); }
    int			nrCrl() const { return nrTrcs(); }

    int			inlIdx( linenr_type lid ) const {return lineIdx(lid);}
    int			crlIdx( trcnr_type tid ) const { return trcIdx(tid); }
    inline void		include( const BinID& bid )
			{ includeLine(bid.inl()); includeTrc(bid.crl()); }
    void		includeInl( int inl ) { includeLine(inl); }
    void		includeCrl( int crl ) { includeTrc(crl); }
    inline bool		includes(const BinID& bid,bool ignoresteps=false) const
			{ return lineOK(bid.inl(),ignoresteps) &&
				 trcOK(bid.crl(),ignoresteps); }
    inline bool		inlOK( int inl ) const { return lineOK(inl); }
    inline bool		crlOK( int crl ) const { return trcOK(crl); }

protected:

    GeomSystem		geomsystem_	= OD::VolBasedGeom;

public:

    void	setIsSynthetic()	{ geomsystem_ = OD::SynthGeom; }
    bool	isSynthetic() const	{ return geomsystem_ == OD::SynthGeom; }

};


mExpClass(Basic) TrcKeySamplingSet : public TypeSet<TrcKeySampling>
{
public:

    mUseType( Pos,	GeomID );

    void		isOK() const;
    void		add(GeomID);
    bool		isPresent(GeomID);
};




/*!
\brief Finds next BinID in TrcKeySampling; initializes to first position.
*/

mExpClass(Basic) TrcKeySamplingIterator
{
public:
		TrcKeySamplingIterator();
		TrcKeySamplingIterator(const TrcKeySampling&);

    void	setSampling(const TrcKeySampling&);
    void	setCurrentPos( od_int64 pos )		{ curpos_ = pos; }
    void	reset();

    bool	next() const;

    TrcKey	curTrcKey() const;
    BinID	curBinID() const;

    od_int64	curIdx() const				{ return curpos_; }
    od_int64	totalNr() const				{ return totalnr_; }

    void	fillPar(IOPar&) const;
    void	usePar(const IOPar&);

protected:

    TrcKeySampling			tks_;
    od_int64				totalnr_;
    mutable Threads::Atomic<od_int64>	curpos_;
};



inline int TrcKeySampling::lineIdx( linenr_type line ) const
{
    return step_.lineNr()
	? (line-start_.lineNr()) / step_.lineNr()
	: (line==start_.lineNr() ? 0 : -1);
}


inline int TrcKeySampling::trcIdx( trcnr_type trcid ) const
{
    return step_.trcNr()
	? (trcid-start_.trcNr()) / step_.trcNr()
	: (trcid==start_.trcNr() ? 0 : -1);
}


inline TrcKeySampling::linenr_type TrcKeySampling::lineID( int lidx ) const
{
    return start_.lineNr() + step_.lineNr() * lidx;
}


inline TrcKeySampling::trcnr_type TrcKeySampling::traceID( int tidx ) const
{
    return start_.trcNr() + step_.trcNr() * tidx;
}

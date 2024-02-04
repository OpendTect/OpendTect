#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posinfo.h"
#include "coord.h"
#include "bufstring.h"
#include "uistring.h"
class TrcKeySampling;
class BinIDSorting;
class BinIDSortingAnalyser;


namespace PosInfo
{

/*!\brief Just hold inl, crl, x, y and offs. For 2D, crl=nr. */

mExpClass(General) CrdBidOffs
{ mODTextTranslationClass(CrdBidOffs);
public:
		CrdBidOffs()
		    : coord_(0,0), binid_(1,0), offset_(0)	{}
		CrdBidOffs( const Coord& c, int nr, float o=0 )
		    : coord_(c), binid_(1,nr), offset_(o)	{}
		CrdBidOffs( const Coord& c, const BinID& b, float o=0 )
		    : coord_(c), binid_(b), offset_(o)		{}
    bool	operator ==( const CrdBidOffs& cbo ) const
		{ return binid_ == cbo.binid_
		      && mIsEqual(offset_,cbo.offset_,mDefEps); }

    Coord	coord_;
    BinID	binid_;
    float	offset_;
    float	azimuth_	= 0;
};


/*!\brief Determines many geometry parameters from a series of Coords with
    corresponding BinID or trace numbers and offsets if prestack. */


mExpClass(General) Detector
{ mODTextTranslationClass(Detector);
public:

    struct Setup
    {
			Setup( bool istwod )
			    : is2d_(istwod), isps_(false)
			    , reqsorting_(false)		{}
	mDefSetupMemb(bool,is2d)
	mDefSetupMemb(bool,isps)
	mDefSetupMemb(bool,reqsorting)
    };

			Detector(const Setup&);
			Detector(const Detector&);
    Detector&		operator =(const Detector&);
			~Detector();

    bool		is2D() const		{ return setup_.is2d_; }
    bool		isPS() const		{ return setup_.isps_; }
    void		reInit();

    bool		add(const Coord&,const BinID&);
    bool		add(const Coord&,const BinID&,float offs);
    bool		add(const Coord&,int nr);
    bool		add(const Coord&,int nr,float offs);
    bool		add(const Coord&,const BinID&,int nr,float offs);
    bool		add(const Coord&,const BinID&,int nr,
			    float offs,float azi);
    bool		add(const CrdBidOffs&);


    bool		finish();
    bool		usable() const		{ return errmsg_.isEmpty(); }
    uiString		errMsg() const		{ return errmsg_; }
    void		appendResults(const Detector&); // after finish only
    void		mergeResults(const Detector&); // after finish only

    int			nrPositions( bool uniq=true ) const
			{ return uniq ? nruniquepos_ : nrpos_; }

			// available after finish()
    Coord		minCoord() const	{ return mincoord_; }
    Coord		maxCoord() const	{ return maxcoord_; }
    BinID		start() const		{ return start_; }
    BinID		stop() const		{ return stop_; }
    BinID		step() const		{ return step_; }
    void		getTrcKeySampling(TrcKeySampling&) const;
    Interval<float>	offsRg() const		{ return offsrg_; }
    Interval<float>	azimuthRg() const	{ return azimuthrg_; }
    float		avgDist() const		{ return avgdist_; }
    CrdBidOffs		firstPosition() const	{ return userCBO(firstcbo_); }
    CrdBidOffs		lastPosition() const	{ return userCBO(lastcbo_); }
    bool		haveGaps( bool inldir=false ) const
			{ return inldir ? inlirreg_ : crlirreg_; }

    void		report(IOPar&) const;

    const BinIDSorting&	sorting() const		{ return sorting_; }
    bool		inlSorted() const;
    bool		crlSorted() const;
    void		getCubeData(CubeData&) const;
			//!< if crlSorted(), inl and crl are swapped
    uiString		getSurvInfoWithMsg(TrcKeySampling&,Coord crd[3]) const;

    StepInterval<int>	getRange(bool inldir=false) const;
    bool		haveStep(bool) const; // to check during detection

protected:

    Setup		setup_;
    BinIDSorting&	sorting_;
    ObjectSet<LineData>	lds_;
    int			nruniquepos_;
    int			nrpos_;
    Coord		mincoord_;
    Coord		maxcoord_;
    Interval<float>	offsrg_;
    int			nroffsperpos_;
    Interval<float>	azimuthrg_;
    bool		allstd_;
    BinID		start_;
    BinID		stop_;
    BinID		step_;
    bool		inlirreg_, crlirreg_;
    Interval<float>	distrg_;	//!< 2D
    float		avgdist_;	//!< 2D
    CrdBidOffs		firstcbo_;
    CrdBidOffs		lastcbo_;
    CrdBidOffs		llnstart_;	//!< in 3D, of longest line
    CrdBidOffs		llnstop_;	//!< in 3D, of longest line
    CrdBidOffs		firstduppos_;
    CrdBidOffs		firstaltnroffs_;

    BinIDSortingAnalyser* sortanal_;
    TypeSet<CrdBidOffs>	cbobuf_;
    int			curline_;
    int			curseg_;
    CrdBidOffs		curcbo_;
    CrdBidOffs		prevcbo_;
    CrdBidOffs		curusrcbo_;
    CrdBidOffs		prevusrcbo_;
    CrdBidOffs		curlnstart_;
    int			nroffsthispos_;
    uiString	        errmsg_;

    bool		applySortAnal();
    void		addFirst(const PosInfo::CrdBidOffs&);
    bool		addNext(const PosInfo::CrdBidOffs&);
    void		addLine();
    void		addPos();
    void		setCur(const PosInfo::CrdBidOffs&);
    void		getBinIDRanges();
    int			getStep(bool inl) const; //!< smallest
    int			getRawStep(bool,bool) const;
    uiString		createPositionString(const CrdBidOffs&) const;

    CrdBidOffs		workCBO(const CrdBidOffs&) const;
    CrdBidOffs		userCBO(const CrdBidOffs&) const;

};


} // namespace PosInfo

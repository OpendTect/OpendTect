#ifndef survinfo_H
#define survinfo_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 Contents:	Features for sets of data
 RCS:		$Id: survinfo.h,v 1.22 2002-06-28 12:57:29 bert Exp $
________________________________________________________________________

-*/
 
 
#include <binid2coord.h>
#include <uidobj.h>
#include <binidselimpl.h>
class ascostream;


/*!\brief Holds survey general information.

The surveyinfo is the primary source for ranges and steps. It also provides
the transformation between inline/xline and coordinates.

Note: the Z range step is only a default. It should not be used further
because different cubes have different sample rates.

The ranges are defined for two cubes: the entire survey, and a 'working area'.
Normally, you'll want to have the working area.

*/

class SurveyInfo : public UserIDObject
{

    friend const SurveyInfo&	SI();

public:

			SurveyInfo(const SurveyInfo&);

    const BinIDRange&	range( bool work=true ) const
    			{ return work ? wrange_ : range_; }
    const BinID&	step( bool work=true ) const
    			{ return work ? wstep_ : step_; }
    const StepInterval<double>& zRange( bool work=true ) const
    			{ return work ? wzrange_ : zrange_; }

    void		setWorkRange( const BinIDRange& b )
			{ setRange( b, true ); }
    void		setWorkStep( const BinID& b )
			{ setStep( b, true ); }
    void		setWorkZRange( const Interval<double>& r )
			{ setZRange( r, true ); }
    void		setWorkZRange( const StepInterval<double>& r )
			{ setZRange( r, true ); }

    void		checkInlRange(Interval<int>&,bool work=true) const;
			//!< Make sure range is inside
    void		checkCrlRange(Interval<int>&,bool work=true) const;
			//!< Make sure range is inside
    void		checkRange(BinIDRange&,bool work=true) const;
			//!< Make sure range is inside
    void		checkZRange(Interval<double>&,bool work=true) const;
			//!< Make sure range is inside

    inline bool		rangeUsable() const
			{ return range_.start.inl && range_.stop.inl
			      && range_.start.crl && range_.stop.crl; }
    inline bool		zRangeUsable() const
			{ return !mIS_ZERO(zrange_.width()); }
    inline bool		zIsTime() const			{ return zistime_; }

    const char*		comment() const			{ return comment_; }

    void		snap(BinID&,BinID rounding=BinID(0,0),
			     bool work=true) const;
			//!< 0 : auto; -1 round downward, 1 round upward
    void		snapStep(BinID&,BinID rounding=BinID(0,0),
	    			 bool work=true) const;
			//!< see snap() for rounding

    inline bool		validTransform() const
			{ return b2c_.isValid(); }
    inline Coord	transform( const BinID& b ) const
			{ return b2c_.transform(b); }
    BinID		transform(const Coord&) const;
    void		get3Pts(Coord c[3],BinID b[2],int& xline) const;
    const BinID2Coord&	binID2Coord() const	{ return b2c_; }

    Coord		minCoord(bool work=true) const;
    bool		isReasonable(const BinID&) const;
			//!< Checks if in or near survey
    inline bool		isReasonable( const Coord& c ) const
			{ return isReasonable( transform(c) ); }

    			// Project name will be stored
    const char*		getWSProjName() const
			{ return wsprojnm_; }
    void		setWSProjName( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wsprojnm_ = nm; }
    			// Password only in memory this session
    const char*		getWSPwd() const
			{ return wspwd_; }
    void		setWSPwd( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wspwd_ = nm; }

    static const char*	sKeyInlRange;
    static const char*	sKeyCrlRange;
    static const char*	sKeyZRange;
    static const char*	sKeyWSProjName;

private:

			SurveyInfo(const char*);
    int			write(const char*) const;

    UserIDString	dirname;
    void		setTr(BinID2Coord::BCTransform&,const char*);
    void		putTr(const BinID2Coord::BCTransform&,	
				ascostream&,const char*) const;

    BinID2Coord		b2c_;
    bool		zistime_;
    BufferString	comment_;
    BufferString	wsprojnm_;
    BufferString	wspwd_;
    BinIDRange		range_, wrange_;
    BinID		step_, wstep_;
    StepInterval<double> zrange_, wzrange_;

    BinID		set3binids[3];
    Coord		set3coords[3];

    static SurveyInfo*	theinst_;
    static bool		dowarnings_;
    static void		produceWarnings( bool yn )	{ dowarnings_ = yn; }

    void		setRange(const BinIDRange&,bool);
    void		setStep(const BinID&,bool);
    void		setZRange(const Interval<double>&,bool);
    void		setZRange(const StepInterval<double>&,bool);
    void		setComment( const char* s )	{ comment_ = s; }
    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);
			//!< returns error message or null on success

    bool		valid_;

    friend class	uiSurvey;
    friend class	uiSurveyInfoEditor;

};


#endif

#ifndef survinfo_H
#define survinfo_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 Contents:	Features for sets of data
 RCS:		$Id: survinfo.h,v 1.17 2002-04-18 09:50:18 bert Exp $
________________________________________________________________________

-*/
 
 
#include <binid2coord.h>
#include <uidobj.h>
#include <binidselimpl.h>
class ascostream;


/*!\brief Holds survey general information.

The surveyinfo is the primary source for ranges and steps. It also provides
the transformation between inline/xline and coordinates.

Note: the Z range step is there only for user information. It should not be
used further because different cubes have different sample rates.

*/

class SurveyInfo : public UserIDObject
{

    friend class		EdSurvey;
    friend class		EdSurveyInfo;
    friend class		uiSurvey;
    friend class		uiSurveyInfoEditor;
    friend const SurveyInfo&	SI();

public:

			SurveyInfo(const SurveyInfo&);

    const BinIDRange&	range() const		{ return range_; }
    void		setRange(const BinIDRange&);
    const BinID&	step() const		{ return step_; }
    void		setStep(const BinID&);
    const StepInterval<double>& zRange() const	{ return zrange_; }
    void		setZRange(const Interval<double>&);
    void		setZRange(const StepInterval<double>&);

    inline bool		rangeUsable() const
			{ return range_.start.inl && range_.stop.inl
			      && range_.start.crl && range_.stop.crl; }
    inline bool		zRangeUsable() const
			{ return !mIS_ZERO(zrange_.width()); }
    inline bool		zIsTime() const			{ return zistime_; }

    void		setComment( const char* s )	{ comment_ = s; }
    const char*		comment() const			{ return comment_; }

    void		snap(BinID&,BinID rounding=BinID(0,0)) const;
			//!< 0 : auto; -1 round downward, 1 round upward
    void		snapStep(BinID&,BinID rounding=BinID(0,0)) const;
			//!< see snap() for rounding

    inline bool		validTransform() const
			{ return b2c_.isValid(); }
    inline Coord	transform( const BinID& b ) const
			{ return b2c_.transform(b); }
    BinID		transform(const Coord&) const;
    void		get3Pts(Coord c[3],BinID b[2],int& xline) const;
    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);
			//!< returns error message or null on success

    const BinID2Coord&	binID2Coord() const	{ return b2c_; }

    Coord		minCoord() const;
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
    BinIDRange		range_;
    BinID		step_;
    StepInterval<double> zrange_;
    bool		zistime_;
    BufferString	comment_;
    BufferString	wsprojnm_;
    BufferString	wspwd_;
    bool		valid_;

    BinID		set3binids[3];
    Coord		set3coords[3];

    static SurveyInfo*	theinst_;

};


#endif

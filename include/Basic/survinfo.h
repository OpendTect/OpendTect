#ifndef survinfo_H
#define survinfo_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 RCS:		$Id: survinfo.h,v 1.26 2002-10-09 15:42:03 bert Exp $
________________________________________________________________________

-*/
 
 
#include <uidobj.h>
#include <binidselimpl.h>
#include <iosfwd>
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

    virtual		~SurveyInfo()			{}

    virtual SurveyInfo*	clone() const			= 0;
    virtual void	copyFrom(const SurveyInfo&);
    virtual bool	is3D() const		{ return true; }

    const BinIDRange&	range( bool work=true ) const
    			{ return work ? wrange_ : range_; }
    const StepInterval<double>& zRange( bool work=true ) const
    			{ return work ? wzrange_ : zrange_; }
    virtual bool	haveStep() const		{ return false; }
    virtual int		getStep(bool inl,bool work=true) const	{ return 1; }
    inline int		inlStep() const		{ return getStep(true,false); }
    inline int		crlStep() const		{ return getStep(false,false); }
    inline int		inlWorkStep() const	{ return getStep(true,true); }
    inline int		crlWorkStep() const	{ return getStep(false,true); }

    void		setWorkRange( const BinIDRange& b )
			{ setRange( b, true ); }
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
    bool		includes(const BinID,const float,bool work=true) const;
			//!< Returns true when pos is inside survey-range

    inline bool		rangeUsable() const
			{ return range_.start.inl && range_.stop.inl
			      && range_.start.crl && range_.stop.crl; }
    inline bool		zRangeUsable() const
			{ return !mIS_ZERO(zrange_.width()); }
    inline bool		zIsTime() const			{ return zistime_; }

    const char*		comment() const			{ return comment_; }

    virtual void	snap(BinID&,BinID rounding=BinID(0,0),
			     bool work=true) const	= 0;
			//!< 0 : auto; -1 round downward, 1 round upward

    virtual Coord	transform(const BinID&) const	= 0;
    virtual BinID	transform(const Coord&) const	= 0;

    Coord		minCoord(bool work=true) const;
    Coord		maxCoord(bool work=true) const;
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

    bool		isValid() const		{ return valid_; }

protected:

			SurveyInfo();
    bool		valid_;

    static SurveyInfo*	read(const char*);
    bool		write(const char*) const;

    BufferString	datadir;
    BufferString	dirname;

    bool		zistime_;
    BufferString	comment_;
    BufferString	wsprojnm_;
    BufferString	wspwd_;
    BinIDRange		range_, wrange_;
    StepInterval<double> zrange_, wzrange_;

    static SurveyInfo*	theinst_;
    static bool		dowarnings_;
    static void		produceWarnings( bool yn )	{ dowarnings_ = yn; }

    void		setRange(const BinIDRange&,bool);
    void		setZRange(const Interval<double>&,bool);
    void		setZRange(const StepInterval<double>&,bool);
    void		setComment( const char* s )	{ comment_ = s; }

    virtual void	setStep(const BinID&,bool)	{}

    virtual void	handleLineRead(const BufferString&,const char*)	{}
    virtual bool	wrapUpRead()				{ return true; }
    virtual void	writeSpecLines(ascostream&) const	{}
    virtual bool	wrapUpWrite(ostream&,const char*) const	{ return true; }

#define mAddSurvInfoFriends \
    friend class	uiSurvey; \
    friend class	uiSurveyInfoEditor

			mAddSurvInfoFriends;

};


#endif

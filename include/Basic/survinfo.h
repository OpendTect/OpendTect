#ifndef survinfo_H
#define survinfo_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 RCS:		$Id: survinfo.h,v 1.44 2004-07-29 16:52:30 bert Exp $
________________________________________________________________________

-*/
 
 
#include "uidobj.h"
#include "ranges.h"
#include "binid2coord.h"
class ascostream;
class IOPar;
class BinID2Coord;
class CubeSampling;


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

    virtual		~SurveyInfo();
    			SurveyInfo(const SurveyInfo&);
    SurveyInfo&		operator =(const SurveyInfo&);

    const CubeSampling&	sampling( bool work=true ) const
    			{ return work ? wcs_ : cs_; }
    StepInterval<int>	inlRange(bool work=true) const;
    StepInterval<int>	crlRange(bool work=true) const;
    const StepInterval<float>& zRange( bool work=true ) const;
    inline int		inlStep(bool work=true) const;
    inline int		crlStep(bool work=true) const;
    virtual int		maxNrTraces(bool work=false) const;

    void		setWorkRange( const CubeSampling& cs )
			{ setRange(cs,true); }

    void		checkInlRange(Interval<int>&,bool work=true) const;
			//!< Makes sure range is inside
    void		checkCrlRange(Interval<int>&,bool work=true) const;
			//!< Makes sure range is inside
    void		checkZRange(Interval<float>&,bool work=true) const;
			//!< Makes sure range is inside
    bool		includes(const BinID&,const float,bool work=true) const;
			//!< Returns true when pos is inside survey-range

    inline bool		zIsTime() const	 { return zistime_; }
    inline bool		zInMeter() const { return !zistime_ && !zinfeet_; }
    inline bool		zInFeet() const	 { return !zistime_ && zinfeet_; }
    void		setZUnit(bool istime,bool infeet=false);
    const char*		getZUnit(bool withparens=true) const;
    float		zFactor() const		{ return zistime_ ? 1000 : 1; }
    			//!< Factor between real and displayed unit

    void		snap(BinID&,BinID direction=BinID(0,0),
	    		     bool work=true) const;
			//!< dir = 0 : auto; -1 round downward, 1 round upward
    void		snapStep(BinID&,BinID direction=BinID(0,0),
	    			 bool work=true) const;
    			//!< see snap() for direction
    void		snapZ(float&,int direction=0,bool work=true) const;
    virtual Coord	transform( const BinID& b ) const
			{ return b2c_.transform(b); }
    virtual BinID	transform(const Coord&) const;
    			/*!<\note The returned BinID will be snapped according
			  	  to the work step. */
    const BinID2Coord&	binID2Coord() const		{ return b2c_; }
    void		get3Pts(Coord c[3],BinID b[2],int& xline) const;

    virtual Coord	minCoord(bool work=true) const;
    virtual Coord	maxCoord(bool work=true) const;
    bool		isReasonable(const BinID&) const;
			//!< Checks if in or near survey
    bool		isReasonable(const Coord&) const;
			//!< Checks if in or near survey

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
    static const char*	sKeyDpthInFt; //!< 'Depth in feet' Y/N (UI default)

    bool		isValid() const		{ return valid_; }
    const char*		comment() const		{ return comment_; }

    			// These fns are commonly not used ...
    void		setRange(const CubeSampling&,bool);
    void		setComment( const char* s )	{ comment_ = s; }
    static void		produceWarnings( bool yn )	{ dowarnings_ = yn; }

    IOPar&		pars() const
			{ return const_cast<SurveyInfo*>(this)->pars_; }
    bool		depthsInFeetByDefault() const;
    bool		isClockWise() const;
    			/*!< Orientation is determined by rotating the
			     inline axis to the crossline axis. */

    bool		write(const char* basedir=0) const;
    			//!< write to .survey file
    void		savePars(const char* basedir=0) const;

protected:

			SurveyInfo();
    bool		valid_;

    static SurveyInfo*	read(const char*);

    BufferString	datadir;
    BufferString	dirname;

    bool		zistime_;
    bool		zinfeet_; //!< only relevant if zistime_
    BufferString	comment_;
    BufferString	wsprojnm_;
    BufferString	wspwd_;
    CubeSampling&	cs_;
    CubeSampling&	wcs_;
    IOPar&		pars_;

    BinID2Coord		b2c_;
    BinID		set3binids[3];
    Coord		set3coords[3];

    static SurveyInfo*	theinst_;
    static bool		dowarnings_;

    void		handleLineRead(const BufferString&,const char*);
    bool		wrapUpRead();
    void		writeSpecLines(ascostream&) const;

    void		setTr(BinID2Coord::BCTransform&,const char*);
    void		putTr(const BinID2Coord::BCTransform&,
	    			ascostream&,const char*) const;

    friend class	uiSurvey;
    friend class	uiSurveyInfoEditor;

    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);

private:

    BinID2Coord::BCTransform	rdxtr;
    BinID2Coord::BCTransform	rdytr;

};


#endif

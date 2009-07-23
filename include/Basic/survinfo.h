#ifndef survinfo_h
#define survinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		9-4-1996
 RCS:		$Id: survinfo.h,v 1.89 2009-07-23 05:44:15 cvsraman Exp $
________________________________________________________________________

-*/
 
 
#include "namedobj.h"
#include "ranges.h"
#include "rcol2coord.h"
#include "enums.h"

class ascostream;
class IOPar;
class CubeSampling;
class LatLong2Coord;


/*!\brief Holds survey general information.

The surveyinfo is the primary source for ranges and steps. It also provides
the transformation between inline/xline <-> coordinates and lat/long estimates.

Note: the Z range step is only a default. It should not be used further
because different cubes have different sample rates.

The ranges are defined for two cubes: the entire survey, and a 'working area'.
Normally, you'll want to have the working area.

If you are an expert, and you feel you need more 'power', you may want to look
at the bottom part of the class too for some more public functions.

*/

mClass SurveyInfo : public NamedObject
{

    mGlobal friend const SurveyInfo&	SI();

public:

			~SurveyInfo();
    bool		isValid() const		{ return valid_; }

    enum Pol2D      	{ No2D=0, Both2DAnd3D=1, Only2D=2 };
    			DeclareEnumUtils(Pol2D);

    const CubeSampling&	sampling( bool work ) const
    			{ return work ? wcs_ : cs_; }
    StepInterval<int>	inlRange(bool work) const;
    StepInterval<int>	crlRange(bool work) const;
    const StepInterval<float>& zRange( bool work ) const;
    int			inlStep() const;
    int			crlStep() const;
    float		inlDistance() const;
    			/*!<\returns the distance for one increment in
			    inline. */
    float		crlDistance() const;
    			/*!<\returns the distance for one increment in
			    crossline. */
    float		zStep() const;
    int			maxNrTraces(bool work) const;

    void		setWorkRange(const CubeSampling&);
    Notifier<SurveyInfo> workRangeChg;

    void		checkInlRange(Interval<int>&,bool work) const;
			//!< Makes sure range is inside
    void		checkCrlRange(Interval<int>&,bool work) const;
			//!< Makes sure range is inside
    void		checkZRange(Interval<float>&,bool work) const;
			//!< Makes sure range is inside
    bool		includes(const BinID&,const float,bool work) const;
			//!< Returns true when pos is inside survey-range

    enum Unit		{ Second, Meter, Feet };
    Unit		xyUnit() const;
    Unit		zUnit() const;
    inline bool		xyInFeet() const	{ return xyinfeet_;}
    inline bool		zIsTime() const		{ return zistime_; }
    inline bool		zInMeter() const	{ return !zistime_ &&!zinfeet_;}
    inline bool		zInFeet() const		{ return !zistime_ && zinfeet_;}
    void		setXYInFeet( bool yn=true ) { xyinfeet_ = yn; }
    void		setZUnit(bool istime,bool infeet=false);
    const char*		getXYUnitString(bool withparens=true) const;
    const char*		getZUnitString(bool withparens=true) const;
    const char*		getZDomainString() const;
    static float	defaultXYtoZScale(Unit zunit,Unit xyunit);
    			/*!<Gives a ballpark figure of how to scale Z to
			    make it comparable to XY. */
    float		zScale() const;
    			/*!<Gives a ballpark figure of how to scale Z to
			    make it comparable to XY. */
    float		zFactor() const;
    			//!< Factor between real and displayed unit in UI
    bool		depthsInFeetByDefault() const;
    void		setSurvDataType( Pol2D typ )	{ survdatatype_ = typ; }
    Pol2D		getSurvDataType() const	{ return survdatatype_; }
    bool		has2D() const;
    bool		has3D() const;
    const char*		comment() const		{ return comment_.buf(); }

    void		snap(BinID&,BinID direction=BinID(0,0)) const;
			//!< dir = 0 : auto; -1 round downward, 1 round upward
    void		snapStep(BinID&,BinID direction=BinID(0,0)) const;
    			//!< see snap() for direction
    void		snapZ(float&,int direction=0) const;
    			//!< see snap() for direction
    Coord		transform( const BinID& b ) const
			{ return b2c_.transform(b); }
    BinID		transform(const Coord&) const;
    			/*!<\note The returned BinID will be snapped according
			  	  to the work step. */
    const RCol2Coord&	binID2Coord() const	{ return b2c_; }
    void		get3Pts(Coord c[3],BinID b[2],int& xline) const;
    const LatLong2Coord& latlong2Coord() const	{ return ll2c_; }
    bool		isClockWise() const;
    			/*!< Orientation is determined by rotating the
			     inline axis to the crossline axis. */
    float		computeAngleXInl() const;
    			/*!< It's the angle (0 to pi/2) between
			     the X-axis and the Inl-axis (not an inline) */

    Coord		minCoord(bool work) const;
    Coord		maxCoord(bool work) const;
    bool		isInside(const BinID&,bool work) const;
    bool		isReasonable(const BinID&) const;
			//!< Checks if in or near survey
    bool		isReasonable(const Coord&) const;
			//!< Checks if in or near survey

    static const char*	sKeyInlRange();
    static const char*	sKeyCrlRange();
    static const char*	sKeyXRange();
    static const char*	sKeyYRange();
    static const char*	sKeyZRange();
    static const char*	sKeyWSProjName();
    static const char*	sKeyXYInFt();
    static const char*	sKeyDpthInFt(); //!< Not used by SI, just a UI default
    static const char*	sKeySurvDataType();

    const IOPar&	pars() const			{ return pars_; }
    			// Project name will be stored
    const char*		getWSProjName() const	{ return wsprojnm_.buf(); }
    			// Password only in memory this session
    const char*		getWSPwd() const	{ return wspwd_.buf(); }
    BufferString	getDirName() const	{ return dirname; }

	// Some fns moved to bottom that have 'no user servicable parts inside'

protected:

			SurveyInfo();
    bool		valid_;

    BufferString	datadir;
    BufferString	dirname;

    bool		zistime_;
    bool		zinfeet_; //!< only relevant if zistime_ equals false
    bool		xyinfeet_; //!< only relevant for a few cases
    BufferString	comment_;
    BufferString	wsprojnm_;
    BufferString	wspwd_;
    CubeSampling&	cs_;
    CubeSampling&	wcs_;
    IOPar&		pars_;

    RCol2Coord		b2c_;
    LatLong2Coord&	ll2c_;
    BinID		set3binids[3];
    Coord		set3coords[3];

    Pol2D		survdatatype_;
    bool		survdatatypeknown_;

    static SurveyInfo*	theinst_;
 
    static void		deleteInstance();
    void		handleLineRead(const BufferString&,const char*);
    bool		wrapUpRead();
    void		writeSpecLines(ascostream&) const;

    void		setTr(RCol2Coord::RCTransform&,const char*);
    void		putTr(const RCol2Coord::RCTransform&,
	    			ascostream&,const char*) const;

private:

    // ugly, but hard to avoid:
    friend class		IOMan;
    friend class		uiSurvey;
    friend class		uiSurveyInfoEditor;

    RCol2Coord::RCTransform	rdxtr;
    RCol2Coord::RCTransform	rdytr;

public:

	// These fns are used by specialist classes. Know what you are doing!

    			SurveyInfo(const SurveyInfo&);
    SurveyInfo&		operator =(const SurveyInfo&);

    RCol2Coord&		getBinID2Coord() const
    			{ return const_cast<SurveyInfo*>(this)->b2c_; }
    LatLong2Coord&	getLatlong2Coord() const
    			{ return const_cast<SurveyInfo*>(this)->ll2c_; }
    IOPar&		getPars() const	
    			{ return const_cast<SurveyInfo*>(this)->pars_; }

    bool		write(const char* basedir=0) const;
    			//!< Write to .survey file
    void		savePars(const char* basedir=0) const;
    			//!< Write to .defs file
    static SurveyInfo*	read(const char*);
    void		setRange(const CubeSampling&,bool);
    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);
    void		setComment( const char* s )	{ comment_ = s; }
    void		setInvalid() const;

    void		setWSProjName( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wsprojnm_ = nm; }
    void		setWSPwd( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wspwd_ = nm; }

};


mGlobal const SurveyInfo& SI();


#endif

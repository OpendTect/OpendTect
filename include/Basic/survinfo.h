#ifndef survinfo_H
#define survinfo_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 RCS:		$Id: survinfo.h,v 1.61 2007-11-07 16:06:10 cvsbert Exp $
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
the transformation between inline/xline and coordinates.

Note: the Z range step is only a default. It should not be used further
because different cubes have different sample rates.

The ranges are defined for two cubes: the entire survey, and a 'working area'.
Normally, you'll want to have the working area.

*/

class SurveyInfo : public NamedObject
{

    friend const SurveyInfo&	SI();

public:

			~SurveyInfo();
    			SurveyInfo(const SurveyInfo&);
    SurveyInfo&		operator =(const SurveyInfo&);

    enum Pol2D      	{ No2D=0, Both2DAnd3D=1, Only2D=2 };
    			DeclareEnumUtils(Pol2D)

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

    inline bool		zIsTime() const		{ return zistime_; }
    inline bool		zInMeter() const	{ return !zistime_ &&!zinfeet_;}
    inline bool		zInFeet() const		{ return !zistime_ && zinfeet_;}
    void		setZUnit(bool istime,bool infeet=false);
    const char*		getZUnit(bool withparens=true) const;
    float		zFactor() const		{ return zistime_ ? 1000 : 1; }
    			//!< Factor between real and displayed unit
    void		setSurvDataType( Pol2D typ )	{ survdatatype_ = typ; }
    Pol2D		getSurvDataType() const	{ return survdatatype_; }
    bool		has2D() const;
    bool		has3D() const;

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
    const RCol2Coord&	binID2Coord() const		{ return b2c_; }
    void		get3Pts(Coord c[3],BinID b[2],int& xline) const;
    const LatLong2Coord& latlong2Coord() const	{ return ll2c_; }

    Coord		minCoord(bool work) const;
    Coord		maxCoord(bool work) const;
    bool		isReasonable(const BinID&) const;
			//!< Checks if in or near survey
    bool		isReasonable(const Coord&) const;
			//!< Checks if in or near survey

    			// Project name will be stored
    const char*		getWSProjName() const	{ return wsprojnm_; }
    void		setWSProjName( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wsprojnm_ = nm; }
    			// Password only in memory this session
    const char*		getWSPwd() const	{ return wspwd_; }
    void		setWSPwd( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wspwd_ = nm; }

    static const char*	sKeyInlRange;
    static const char*	sKeyCrlRange;
    static const char*	sKeyZRange;
    static const char*	sKeyWSProjName;
    static const char*	sKeyDpthInFt; //!< 'Depth in feet' Y/N (UI default)
    static const char*	sKeySurvDataType;

    bool		isValid() const		{ return valid_; }
    const char*		comment() const		{ return comment_; }

    			// These fns are commonly not used ...
    static SurveyInfo*	read(const char*);
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
    			//!< Write to .survey file
    void		savePars(const char* basedir=0) const;
    			//!< Write to .defs file

protected:

			SurveyInfo();
    bool		valid_;

    BufferString	datadir;
    BufferString	dirname;

    bool		zistime_;
    bool		zinfeet_; //!< only relevant if zistime_ equals false
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
    static bool		dowarnings_;

    void		handleLineRead(const BufferString&,const char*);
    bool		wrapUpRead();
    void		writeSpecLines(ascostream&) const;

    void		setTr(RCol2Coord::RCTransform&,const char*);
    void		putTr(const RCol2Coord::RCTransform&,
	    			ascostream&,const char*) const;

    friend class	uiSurvey;
    friend class	uiSurveyInfoEditor;
    friend class	IOMan;

    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);

private:

    RCol2Coord::RCTransform	rdxtr;
    RCol2Coord::RCTransform	rdytr;

};


const SurveyInfo&	SI();


#endif

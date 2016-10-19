#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		9-4-1996
________________________________________________________________________

-*/

#include "basicmod.h"
#include "namedobj.h"
#include "ranges.h"
#include "enums.h"
#include "zdomain.h"
#include "notify.h"
#include "survgeom3d.h"

class ascostream;

namespace Coords { class PositionSystem; }


/*!
\brief Holds survey general information.

  The surveyinfo is the primary source for ranges and steps.It also provides
  the transformation between inline/xline <-> coordinates and lat/long
  estimates.

  Note: the Z range step is only a default. It should not be used further
  because different cubes/lines have different sample rates.

  The ranges are defined for two cubes: the entire survey, and a 'working area'.
  Normally, you'll want to have the working area.

  To keep things safe we have made the non-const stuff inaccessible for casual
  use. On non-shared objects, you can do as you like wth all the non-const
  functions. The SI(), that is shared by all objects, is a const reference. If
  you have to edit something in your own local copy, cast the const away.

  The 'major' changes are thus done only via assignment, for SI() preferably by
  specialist classes.

  This does not need to be done for the 'minor' changes like work sampling,
  defaultPars() and comment(). These can be changed at any time and the regular
  'change-and-notify' will apply - even while the functions are 'const'
  functions.

*/

mExpClass(Basic) SurveyInfo : public NamedMonitorable
{ mODTextTranslationClass(SurveyInfo);

    mGlobal(Basic) friend const SurveyInfo&	SI();


public:

    bool		has2D() const;
    bool		has3D() const;

    StepInterval<int>	inlRange(bool work) const;
    StepInterval<int>	crlRange(bool work) const;
    StepInterval<float>	zRange(bool work) const;
    TrcKeyZSampling	sampling(bool work) const;
    int			inlStep() const;
    int			crlStep() const;
    float		zStep() const;
    float		inlDistance() const; //!< distance for one increment
    float		crlDistance() const;

    Coord		transform(const BinID&) const;
    BinID		transform(const Coord&) const;
			/*!<\note BinID will be snapped using work step. */

    bool		xyInFeet() const;
    uiString		xyUnitString(bool abbrviated=true,
				     bool withparens=true) const;
    const char*		fileXYUnitString(bool withparens=true) const;
    const ZDomain::Def&	zDomain() const;
    float		showZ2UserFactor() const;
    uiString		zUnitString(bool withparens=true) const;
    const char*		fileZUnitString(bool withparens=true) const;
    mImplSimpleMonitoredGetSet(inline,depthsInFeet,setDepthsInFeet,
				bool,depthsinfeet_,cParsChange());
			// convenience (can be asked directly from zDomain)
    bool		zIsTime() const;
    bool		zInMeter() const;
    bool		zInFeet() const;

    Coord		minCoord(bool work) const;
    Coord		maxCoord(bool work) const;
    bool		isInside(const BinID&,bool work) const;
    bool		isReasonable(const BinID&) const;
				//!< Checks if in or near survey
    bool		isReasonable(const Coord&) const;
				//!< Checks if in or near survey
    Interval<int>	reasonableRange(bool inl) const;
    int			maxNrTraces(bool work) const;
    bool		isRightHandSystem() const;
			/*!< rotating the inline axis to the crossline axis. */
    float		angleXInl() const;
			/*!< angle between the X-axis (East) and an Inline */

    void		checkInlRange(Interval<int>&,bool work) const;
    void		checkCrlRange(Interval<int>&,bool work) const;
    void		checkZRange(Interval<float>&,bool work) const;
    bool		includes(const BinID&,const float,bool work) const;

    void		snap(BinID&,const BinID& dir=BinID(0,0)) const;
			//!< dir = 0 : auto; -1 round downward, 1 round upward
    void		snapStep(BinID&,const BinID& dir=BinID(0,0))const;
			//!< see snap() for direction
    void		snapZ(float&,int direction=0) const;
			//!< see snap() for direction

    mImplSimpleMonitoredGetSet(inline,
			seismicReferenceDatum,setSeismicReferenceDatum,
			float,seisrefdatum_,cAuxDataChange());
			/*!< is in depth units (m or ft), positive upward
			    from sea level. Always in meters for time surveys */

    IOPar		defaultPars() const;
    void		setDefaultPar(const char* ky,const char* val,
					bool save2storage) const;
    void		setDefaultPars(const IOPar&,bool save2storage) const;
    void		removeKeyFromDefaultPars(const char* ky,
						 bool save2storage) const;
    void		putZDomain(IOPar&) const;
    void		setWorkRange(const TrcKeyZSampling&) const;
    void		getCreationData(IOPar&) const;
			//!< std creation entries and some SIP stuff

    RefMan<Survey::Geometry3D>		get3DGeometry(bool work) const;
    RefMan<Coords::PositionSystem>	getCoordSystem();
    ConstRefMan<Coords::PositionSystem> getCoordSystem() const;

    enum Pol2D		{ No2D=0, Both2DAnd3D=1, Only2D=2 };
			mDeclareEnumUtils(Pol2D);

    float		getArea(Interval<int> inl,Interval<int> crl) const;
				//!<returns square meters
    float		getArea(bool work) const;   //!<returns square meters
    Coord3		oneStepTranslation(const Coord3& planenormal) const;

			// Contrary to normal assignment, only one of
			// 'major' changes will be emitted.
    static ChangeType	cSetupChange()		{ return 2; }
    static ChangeType	cRangeChange()		{ return 3; }
    static ChangeType	cWorkRangeChange()	{ return 4; }

			// minor changes will be sent only if not a 'major' one
			// is emitted.
    static ChangeType	cParsChange()		{ return 6; }
    static ChangeType	cPol2DChange()		{ return 7; }
    static ChangeType	cAuxDataChange()	{ return 8; }
    static ChangeType	cCommentChange()	{ return 9; }

    static bool		isMinorChange( ChangeType ct )
			{ return ct==cNameChange() || ct>cRangeChange(); }

protected:

    const BufferString	uniqueid_;
    BufferString	basepath_;	//!< The 'data root'
    BufferString	dirname_;	//!< The subdirectory name
    ZDomain::Def&	zdef_;
    bool		depthsinfeet_;
    TrcKeyZSampling&	fullcs_;
    TrcKeyZSampling&	workcs_;
    float		seisrefdatum_;
    IOPar		defpars_;
    RefMan<Coords::PositionSystem> coordsystem_;

    Survey::Geometry3D*	s3dgeom_;
    Survey::Geometry3D*	work_s3dgeom_;

    Pos::IdxPair2Coord	b2c_;

    BinID		set3binids_[3];
    Coord		set3coords_[3];

    Pol2D		pol2d_;
    bool		pol2dknown_;

    BufferString	comments_;
    BufferString	sipnm_;

    bool		wrapUpRead();

    mImplSimpleMonitoredGetSet(inline,comments,setComments,
				BufferString,comments_,cCommentChange());
    TrcKeyZSampling&	gtSampling( bool work ) const
			{ return work ? workcs_ : fullcs_; }

private:

    // ugly, but hard to avoid:
    friend class	DBMan;
    friend class	uiSurveyManager;
    friend class	uiSurveyInfoEditor;

    Pos::IdxPair2Coord::DirTransform rdxtr_;
    Pos::IdxPair2Coord::DirTransform rdytr_;

				// For DBMan only
    static uiRetVal	setSurveyLocation(const char*,const char*);

public:

	// These fns are rarely used by non-specialist classes.

			SurveyInfo();
			~SurveyInfo();
    mDeclMonitorableAssignment(SurveyInfo);

    Pos::IdxPair2Coord	binID2Coord() const;
    void		get3Pts(Coord c[3],BinID b[2],int& xline) const;

    mDeprecated bool	isClockWise() const { return isRightHandSystem(); }
    void		setZUnit(bool istime,bool infeet=false);

    static const char*	sKeyInlRange();
    static const char*	sKeyCrlRange();
    static const char*	sKeyXRange();
    static const char*	sKeyYRange();
    static const char*	sKeyZRange();
    static const char*	sKeyXYInFt();
    static const char*	sKeyDpthInFt(); //!< Not used by SI, just a UI default
    static const char*	sKeySurvDataType();
    static const char*  sKeySeismicRefDatum();
    static const char*	sSetupFileName()		{ return ".survey"; }
    static const char*	sBasicSurveyName()		{ return "BasicSurvey";}

    mImplSimpleMonitoredGetSet(inline,getDirName,setDirName,
				BufferString,dirname_,cSetupChange());
    mImplSimpleMonitoredGetSet(inline,getBasePath,setBasePath,
				BufferString,basepath_,cSetupChange());
    BufferString	getFullDirPath() const;
    static BufferString	dirNameForName(const char*);

    Pol2D		survDataType() const;
    void		setSurvDataType(Pol2D) const;
    mImplSimpleMonitoredGetSet(inline,sipName,setSipName,
				BufferString,sipnm_,cAuxDataChange());

	// Following fns are used by specialist classes. Don't use casually.

    bool		write(const char* basedir=0) const;
			//!< Write to .survey file and .defs file
    void		saveDefaultPars(const char* basedir=0) const;
			//!< Write to .defs file
    void		saveComments(const char* basedir=0) const;
			//!< Write to .comments file
    static SurveyInfo*	read(const char*,uiRetVal&);
    void		setRange(const TrcKeyZSampling&);
    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);
    const uiString	set3PtsUiMsg(const Coord c[3],const BinID b[2],int);
    void		gen3Pts();
    bool		setCoordSystem(Coords::PositionSystem*);
    void		update3DGeometry();

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    static uiRetVal	isValidDataRoot(const char*);
    static uiRetVal	isValidSurveyDir(const char*);

    ChangeType		mainDiff(const SurveyInfo&) const;
			//< returns cEntireObjectChangeType() for other survey
			//< returns mUdf(ChangeType) for no change

    bool		isFresh() const;
    void		setNotFresh() const;
    void		setFreshSetupData(const IOPar&) const;
    void		getFreshSetupData(IOPar&) const;

    mDeprecated IOPar&		defaultPars()
				{ return defpars_; }
    mDeprecated const IOPar&	pars() const
				{ return defpars_; }
    mDeprecated void		savePars(const char* basedir = 0) const
				{ saveDefaultPars(basedir); }
    mDeprecated IOPar&		getPars() const
				{ return const_cast<IOPar&>(defpars_); }
    mDeprecated BufferString	getDataDirName() const
				{ return getBasePath();}

    mDeprecated void		setRange( const TrcKeyZSampling& cs, bool work )
				{ work ? setWorkRange(cs) : setRange(cs); }
    mDeprecated const char*	getZUnitString( bool wp=true ) const
				{ return fileZUnitString( wp ); }
    mDeprecated uiString	getUiZUnitString( bool wp=true ) const
				{ return zUnitString( wp ); }
    mDeprecated const char*	getXYUnitString( bool wp=true ) const
				{ return fileXYUnitString( wp ); }
    mDeprecated uiString	getUiXYUnitString(bool a=true,bool p=true) const
				{ return xyUnitString( a, p ); }

    enum Unit		{ Second, Meter, Feet };
    Unit		xyUnit() const;
    Unit		zUnit() const;
    static float	defaultXYtoZScale(Unit,Unit);
			/*!<Gives a ballpark figure of how to scale XY to
			    make it comparable to Z. */
    float		zScale() const;
			/*!<Gives a ballpark figure of how to scale Z to
			    make it comparable to XY. */

};


mGlobal(Basic) const SurveyInfo& SI();
mDeprecated mGlobal(Basic) SurveyInfo& eSI();

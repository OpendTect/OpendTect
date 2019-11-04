#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		9-4-1996
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "enums.h"
#include "namedmonitorable.h"
#include "posidxpair2coord.h"
#include "ranges.h"
#include "surveydisklocation.h"
#include "zdomain.h"

class CubeSampling;
class CubeSubSel;
class HorSampling;
namespace Coords { class CoordSystem; }
namespace Survey { class Geometry; class Geometry3D; class GeometryManager; }
class TrcKeySampling;
class TrcKeyZSampling;

#define mSurvLimArg SurvLimitType lt=OD::FullSurvey

/*!\brief Holds survey general information.

  The surveyinfo is the primary source for ranges and steps.It also provides
  the transformation between inline/xline <-> coordinates and lat/long
  estimates.

  Note: the Z range step is only a default. It should not be used further
  because different cubes/lines have different sample rates.

  The ranges are defined for two cubes: the entire survey, and a 'working area'.
  Normally, you'll want to have the OD::FullSurvey in non-UI, OD::UsrWork
  in UI settings.

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

    mGlobal(Basic) friend const SurveyInfo&	SI(const SurveyInfo*);


public:

    typedef Pos::Index_Type	idx_type;
    typedef idx_type		pos_type;
    typedef idx_type		size_type;
    typedef Pos::rg_type	pos_rg_type;
    typedef Pos::steprg_type	pos_steprg_type;
    typedef Pos::Z_Type		z_type;
    typedef ZSampling		z_steprg_type;
    typedef Pos::Distance_Type	dist_type;
    typedef dist_type		area_type;
    mUseType( OD,		SurvLimitType );
    mUseType( Survey,		Geometry3D );
    mUseType( Coords,		CoordSystem );
    mUseType( Pos,		IdxPair2Coord );

    bool		has2D() const;
    bool		has3D() const;

    Coord		transform(const BinID&) const;
    BinID		transform(const Coord&,mSurvLimArg) const;

    pos_steprg_type	inlRange(mSurvLimArg) const;
    pos_steprg_type	crlRange(mSurvLimArg) const;
    z_steprg_type	zRange(mSurvLimArg) const;
    pos_type		inlStep(mSurvLimArg) const;
    pos_type		crlStep(mSurvLimArg) const;
    z_type		zStep(mSurvLimArg) const;
    BinID		steps(mSurvLimArg) const;

    void		getHorSampling(HorSampling&,mSurvLimArg) const;
    void		getCubeSampling(CubeSampling&,mSurvLimArg) const;
    void		getSampling(TrcKeySampling&,mSurvLimArg) const;
    void		getSampling(TrcKeyZSampling&,mSurvLimArg) const;

    dist_type		inlDistance() const; // between a step 1 in inls!
    dist_type		crlDistance() const; // between a step 1 in crls!
    Coord		distances() const;

    bool		xyInFeet() const;
    uiString		xyUnitString(bool abbrviated=true) const;
    const char*		fileXYUnitString(bool withparens=true) const;
    const ZDomain::Def&	zDomain() const;
    z_type		showZ2UserFactor() const;
    uiString		zUnitString() const; // No parentheses. Use withUnit()
    const char*		fileZUnitString(bool withparens=true) const;
    mImplSimpleMonitoredGetSet(inline,depthsInFeet,setDepthsInFeet,
				bool,depthsinfeet_,cParsChange());
			// convenience (can be asked directly from zDomain)
    bool		zIsTime() const;
    bool		zInMeter() const;
    bool		zInFeet() const;
			//!< If zInFeet, prepare for ALL z's to be in feet,
			//!< in ALL objects
			//!< when displaying, honor depthsInFeet() though!

    Coord		minCoord(mSurvLimArg) const;
    Coord		maxCoord(mSurvLimArg) const;
    bool		isReasonable(const BinID&) const;
				//!< Checks if in or near survey
    bool		isReasonable(const Coord&) const;
				//!< Checks if in or near survey
    Interval<pos_type>	reasonableRange(bool inl) const;
    size_type		maxNrTraces(mSurvLimArg) const;
    bool		isRightHandSystem() const;
			/*!< rotating the inline axis to the crossline axis. */
    float		angleXInl() const;
			/*!< angle between the X-axis (East) and an Inline */
    float		angleXCrl() const;
			/*!< angle between the X-axis (East) and a Crossline */

    void		checkInlRange(Interval<pos_type>&,mSurvLimArg) const;
    void		checkCrlRange(Interval<pos_type>&,mSurvLimArg) const;
    void		checkZRange(Interval<z_type>&,mSurvLimArg) const;
    bool		includes(const BinID&,mSurvLimArg) const;
    bool		includes(const BinID&,z_type,mSurvLimArg) const;

    void		snap(BinID&,OD::SnapDir =OD::SnapNearest) const;
    void		snapStep(BinID&)const;
    void		snapZ(z_type&,OD::SnapDir dir=OD::SnapNearest) const;

    mImplSimpleMonitoredGetSet(inline,
			seismicReferenceDatum,setSeismicReferenceDatum,
			z_type,seisrefdatum_,cAuxDataChange());
			/*!< is in depth units (m or ft), positive upward
			    from sea level. Always in meters for time surveys */

    IOPar		getDefaultPars() const;
    void		setDefaultPar(const char* ky,const char* val,
					bool save2storage) const;
    void		setDefaultPars(const IOPar&,bool save2storage) const;
    void		removeKeyFromDefaultPars(const char* ky,
						 bool save2storage) const;
    void		putZDomain(IOPar&) const;
    void		getCreationData(IOPar&) const;
				//!< std creation entries and some SIP stuff

    RefMan<Geometry3D>	get3DGeometry(mSurvLimArg);
    ConstRefMan<Geometry3D> get3DGeometry(mSurvLimArg) const;
    RefMan<CoordSystem>	getCoordSystem();
    ConstRefMan<CoordSystem> getCoordSystem() const;

    typedef OD::Pol2D3D	Pol2D3D;
			mDeclareEnumUtils(Pol2D3D);

				//!< areas in square meters
    area_type		getArea(Interval<pos_type> inl,
				Interval<pos_type> crl) const;
    area_type		getArea(mSurvLimArg) const;
    Coord3		oneStepTranslation(const Coord3& planenormal) const;

			// Contrary to normal assignment, only one of
			// 'major' changes will be emitted.
    static ChangeType	cSetupChange()		{ return 2; }
    static ChangeType	cRangeChange()		{ return 3; }
    static ChangeType	cWorkRangeChange()	{ return 4; }

			// minor changes will be sent only if not a 'major' one
			// is emitted.
    static ChangeType	cParsChange()		{ return 6; }
    static ChangeType	cPol2D3DChange()	{ return 7; }
    static ChangeType	cAuxDataChange()	{ return 8; }
    static ChangeType	cCommentChange()	{ return 9; }

    static bool		isMinorChange( ChangeType ct )
			{ return ct==cNoChange() || ct==cNameChange()
			      || ct>cRangeChange(); }
    static bool		isSetupChange( ChangeType ct )
			{ return ct==cEntireObjectChange()
			      || ct==cSetupChange(); }

protected:

    SurveyDiskLocation	diskloc_;
    ZDomain::Def&	zdef_;
    bool		depthsinfeet_;
    z_type		seisrefdatum_;
    IOPar		defpars_;
    BufferString	comments_;

    Geometry3D*		s3dgeom_;
    Geometry3D*		work_s3dgeom_;
    RefMan<CoordSystem>	coordsystem_;

    IdxPair2Coord	b2c_;
    BinID		set3binids_[3];
    Coord		set3coords_[3];
    Pol2D3D		pol2d3d_;
    bool		pol2d3dknown_;
    uiString		sipnm_;

    bool		wrapUpRead();

private:

			// ugly, but hard to avoid:
    friend class	DBMan;
    friend class	Survey::Geometry;
    friend class	uiSurveyManager;
    friend class	uiSurveyInfoEditor;
    friend class	uiSurvInfoProvider;

    IdxPair2Coord	rdb2c_;
    CubeSampling&	fullcs_;
    CubeSampling&	workcs_;

    mutable Threads::Lock make3dgeomlock_;
    Geometry3D&		gt3DGeom(mSurvLimArg) const;
    void		gen3Pts();
    void		updateGeometries();
    void		updateGeometry(Geometry3D&,const CubeSampling&);

    void		setToUnlocatedCoordSys(bool);

public:

	// These fns are rarely used by non-specialist classes.

			SurveyInfo();
			~SurveyInfo();
    mDeclMonitorableAssignment(SurveyInfo);

    Pos::IdxPair2Coord	binID2Coord() const;
    void		get3Pts(Coord c[3],BinID b[2],pos_type& xline) const;
    void		setZUnit(bool istime,bool infeet=false);
    void		setRanges(const CubeSampling&);

    static const char*	sKeyInlRange();
    static const char*	sKeyCrlRange();
    static const char*	sKeyXRange();
    static const char*	sKeyYRange();
    static const char*	sKeyZRange();
    static const char*	sKeyXYInFt();
    static const char*	sKeyDpthInFt(); //!< Not used by SI, just a UI default
    static const char*	sKeySurvDataType();
    static const char*  sKeySeismicRefDatum();

    static uiString	sInlRange();
    static uiString	sCrlRange();
    static uiString	sXRange();
    static uiString	sYRange();
    static uiString	sZRange();
    static uiString	sXYInFt();
    static uiString	sDpthInFt(); //!< Not used by SI, just a UI default
    static uiString	sSurvDataType();
    static uiString	sSeismicRefDatum();

    static const char*	sSetupFileName()		{ return ".survey"; }
    static const char*	sBasicSurveyName()		{ return "BasicSurvey";}

    BufferString	dirName() const	{ return diskLocation().dirName(); }
    BufferString	basePath() const { return diskLocation().basePath(); }
    BufferString	getFullDirPath() const;
    mImplSimpleMonitoredGetSet(inline,diskLocation,setDiskLocation,
				SurveyDiskLocation,diskloc_,cSetupChange());
    static BufferString	dirNameForName(const char*);

    Pol2D3D		survDataType() const;
    void		setSurvDataType(Pol2D3D) const;
    mImplSimpleMonitoredGet(sipName,uiString,sipnm_);
    void		setSipName(const uiString&);

	// Following fns are used by specialist classes. Don't use casually.

    void		setWorkRanges(const CubeSubSel&) const;
    bool		write(const char* basedir=0) const;
			//!< Write to .survey file and .defs file
    void		saveDefaultPars(const char* basedir=0) const;
			//!< Write to .defs file
    void		saveComments(const char* basedir=0) const;
			//!< Write to .comments file
    static SurveyInfo*	read(const char*,uiRetVal&);
    uiString		set3Pts(const Coord c[3],const BinID b[2],Index_Type);
    bool		setCoordSystem(Coords::CoordSystem*);
    void		readSavedCoordSystem() const;
			//!< Useful after loading plugins.

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    static uiRetVal	isValidDataRoot(const char*);
    static uiRetVal	isValidSurveyDir(const char*);

    bool		isFresh() const;
    void		setNotFresh() const;
    void		setFreshSetupData(const IOPar&) const;
    void		getFreshSetupData(IOPar&) const;

    mImplSimpleMonitoredGetSet(inline,comments,setComments,
				BufferString,comments_,cCommentChange());

    enum Unit		{ Second, Meter, Feet };
    Unit		xyUnit() const;
    Unit		zUnit() const;
    static z_type	defaultXYtoZScale(Unit,Unit);
			/*!<Gives a ballpark figure of how to scale XY to
			    make it comparable to Z. */
    z_type		zScale() const;
			/*!<Gives a ballpark figure of how to scale Z to
			    make it comparable to XY. */

    mDeprecated const IOPar&	pars() const { return defpars_; }
    mDeprecated void		savePars(const char* basedir = 0) const
					{ saveDefaultPars(basedir); }
    mDeprecated IOPar&		getPars() const
					{ return const_cast<IOPar&>(defpars_); }
    mDeprecated BufferString	getDataDirName() const
					{ return basePath();}

    mDeprecated const char*	getZUnitString( bool wthparens=true ) const
				{ return fileZUnitString( wthparens ); }
    mDeprecated const char*	getXYUnitString( bool wthparens=true ) const
				{ return fileXYUnitString( wthparens ); }
    mDeprecated bool		isInside( const BinID& b, bool work ) const
    { return includes( b, work ? OD::UsrWork : OD::FullSurvey ); }

				// For DBMan and programs in Basic and Algo
    static uiRetVal	setSurveyLocation(const SurveyDiskLocation&,bool);

};


mGlobal(Basic) const SurveyInfo& SI(const SurveyInfo* si=nullptr);
mDeprecated mGlobal(Basic) SurveyInfo& eSI();

namespace Survey {

mGlobal(Basic) void getDirectoryNames(BufferStringSet&,bool fullpath,
			       const char* dataroot=0,const char* excludenm=0);

} // namespace Survey

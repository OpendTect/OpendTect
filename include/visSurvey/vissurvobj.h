#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "vissurveycommon.h"
#include "attribsel.h"
#include "color.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "gendefs.h"
#include "dbkey.h"
#include "position.h"
#include "ranges.h"
#include "survgeom3d.h"
#include "vissurvscene.h"
#include "factory.h"
#include "coltabmappersetup.h"

class DataPointSet;
namespace OD { class LineStyle; class MarkerStyle3D; };
class NotifierAccess;
class RegularSeisDataPack;
class SeisTrcBuf;
class ZAxisTransform;
class TaskRunner;
class TaskRunnerProvider;
namespace ColTab  { class Sequence; }
namespace visBase
{
    class Transformation;
    class EventInfo;
    class TextureChannels;
    class TextureChannel2RGBA;
};


namespace visSurvey
{

/*!\brief Base class for all 'Display' objects */

mExpClass(visSurvey) SurveyObject
{ mODTextTranslationClass(SurveyObject)
public:
    void			doRef();
    void			doUnRef();

				mDefineFactoryInClass(SurveyObject,factory);

    virtual void		set3DSurvGeom(const SurvGeom3D*);
    const SurvGeom3D*		get3DSurvGeom() const { return s3dgeom_; }
    virtual const char*		get3DSurvGeomName() const;
    virtual Pos::GeomID		getGeomID() const;

    virtual Coord3		getNormal(const Coord3& pos) const
				{ return Coord3::udf(); }
				/*!<Position and Normal are both in
				    displayspace. */
    virtual float		calcDist(const Coord3& pos) const
				{ return mUdf(float); }
				/*<\Calculates distance between pick and
				    object.
				    \note The distance is in display space.
				    \param pos Position to be checked in
					   displayspace.
				 \ */
    virtual float		maxDist() const		{ return sDefMaxDist();}
				/*<\Returns maximum allowed distance between
				    pick and object. If calcDist() > maxDist()
				    pick will not be displayed. */
    virtual bool		allowsPicks() const	{ return false; }
				/*<\Returns whether picks can be created
				    on object. */
    virtual bool		isPicking() const	{ return false; }
				/*<\Returns true if object is in a mode
				    where clicking on other objects are
				    handled by object itself, and not passed
				    on to selection manager .*/
    virtual void		getPickingMessage( uiString& msg ) const
				{ msg = tr("Picking"); }

    virtual void		snapToTracePos(Coord3&)	const {}
				//<\Snaps coordinate to a trace position

    virtual NotifierAccess*	getMovementNotifier()	{ return 0; }
				/*!<Gives access to a notifier that is triggered
				    when object is moved or modified. */

    virtual void		otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    int whichobj ) {}
				/*!< If other objects are moved, removed or
				     added in the scene, this function is
				     called. \note that it only notifies on
				     objects that return something on
				     getMovementNotifier().
				     \param whichobj refers to id of the
				     moved object */

    virtual void		setTranslation(const Coord3&) {}
    virtual Coord3		getTranslation() const
				{ return Coord3(0,0,0); }

    virtual void		getChildren( TypeSet<int>& ) const	{}

    virtual bool		canDuplicate() const	{ return false;}
    virtual SurveyObject*	duplicate(TaskRunner*) const	{ return 0; }

    virtual DBKey		getDBKey() const { return DBKey::getInvalid(); }

    virtual bool		hasPosModeManipulator() const	{ return false;}
    virtual void		showManipulator(bool yn)	{}
    virtual bool		isManipulatorShown() const	{ return false;}
    virtual bool		isManipulated() const		{ return false;}
    virtual bool		canResetManipulation() const	{ return false;}
    virtual void		resetManipulation()		{}
    virtual void		acceptManipulation()		{}
    virtual BufferString	getManipulationString() const	{ return ""; }
    virtual NotifierAccess*	getManipulationNotifier()	{ return 0; }

    virtual bool		allowMaterialEdit() const	{ return false;}
				/*!\note Modification of color should be done
					 with setMaterial on
					 visBase::VisualObject */

    virtual const OD::LineStyle*	lineStyle() const { return 0; }
				/*!<If the linestyle can be set, a non-zero
				    pointer should be return. */
    virtual void		setLineStyle(const OD::LineStyle&) {}
    virtual void		getLineWidthBounds(int& min,int& max);
    virtual bool		hasSpecificLineColor() const { return false; }
				/*!<Specifies wether setLineStyle takes
				    regard to the color of the linestyle. */

    virtual void		setMarkerStyle(const OD::MarkerStyle3D&){};
    virtual const OD::MarkerStyle3D*
				markerStyle() const { return 0; }
				/*!<If the marker style can be set, a non-zero
				    pointer should be return. */
    virtual bool		markerStyleColorSelection() const
							    { return true; }
    virtual void		setOnlyAtSectionsDisplay(bool)		{}
    virtual bool		displayedOnlyAtSections() const { return false;}

    virtual bool		hasColor() const	{ return false;}
    virtual bool		usesColor() const	{ return hasColor(); }
    virtual void		setColor(Color)		{}
    virtual Color		getColor() const
				{ return Color::DgbColor(); }

    virtual void		setAnnotColor(Color)			{}
    virtual Color		getAnnotColor() const
				{ return Color::DgbColor(); }

    virtual int			nrResolutions() const		{ return 1; }
    virtual uiWord		getResolutionName(int) const;
    virtual int			getResolution() const		{ return 0; }
    virtual void		setResolution(int,TaskRunner*)	{}

    virtual visBase::TextureChannels* getChannels() const	{ return 0; }
    virtual visBase::TextureChannel2RGBA* getChannels2RGBA()	{ return 0; }
    const visBase::TextureChannel2RGBA*   getChannels2RGBA() const;
    virtual bool		setChannels2RGBA(visBase::TextureChannel2RGBA*)
				{ return false; }

    enum AttribFormat		{ None, Cube, Traces, RandomPos, OtherFormat };
				/*!\enum AttribFormat
					 Specifies how the object wants it's
					 attrib data delivered.
				   \var None
					This object does not handle attribdata.
				   \var	Cube
					This object wants attribdata as
					DataCubes.
				   \var	Traces
					This object wants a set of traces.
				   \var RandomPos
					This object wants a table with
					array positions.
				   \var OtherFormat
					This object wants attribdata of a
					different kind. */

    virtual AttribFormat	getAttributeFormat(int attrib=-1) const;
    virtual bool		canHaveMultipleAttribs() const { return false; }
    virtual int			nrAttribs() const;
    virtual bool		canAddAttrib(int nrattribstoadd=1) const;
    virtual bool		addAttrib()		   { return false; }
    virtual bool		canRemoveAttrib() const;
    virtual bool		removeAttrib(int attrib)   { return false; }
    virtual bool		swapAttribs(int a0,int a1) { return false; }
    virtual void		setAttribTransparency(int,unsigned char) {}
    virtual unsigned char	getAttribTransparency(int) const { return 0; }
    void			getChannelName(int,uiString&) const;
				//!<\returns "Layer 0", or "Red", "Green" ...
    virtual const ColTab::Mapper& getColTabMapper(int attrib) const;
    virtual void		setColTabMapper(int,const ColTab::Mapper&,
						TaskRunner* t=0);
    virtual const ColTab::Sequence& getColTabSequence(int) const;
    virtual bool		canSetColTabSequence() const	{ return false;}
    virtual void		setColTabSequence(int,const ColTab::Sequence&,
						  TaskRunner* t=0);
    virtual bool		canHandleColTabSeqTrans(int) const;

    virtual void		enableTextureInterpolation(bool)	{}
    virtual bool		textureInterpolationEnabled() const
				{ return true; }
    virtual bool		canEnableTextureInterpolation() const
				{ return false; }

    virtual bool		isAngle(int attrib) const	 {return false;}
    virtual void		setAngleFlag(int attrib,bool yn)	{}
    virtual void		enableAttrib(int attrib,bool yn)	{}
    virtual bool		isAttribEnabled(int attrib) const {return true;}
    bool			isAnyAttribEnabled() const;
    virtual bool		hasSingleColorFallback() const { return false; }
    virtual OD::Pol2D3D		getAllowedDataType() const
				{ return OD::Only3D; }

    virtual bool		canRemoveSelection() const	{ return false;}
    virtual bool		removeSelections(TaskRunner*)	{ return false;}
    virtual void		clearSelections()		{}

    virtual void		setSelSpec(int attrib,const Attrib::SelSpec&);
    virtual void		setSelSpecs(int attrib,
					    const Attrib::SelSpecList&);

    virtual const Attrib::SelSpec* getSelSpec( int attrib, int version=0 ) const
				   { return 0; }
    virtual const Attrib::SelSpecList* getSelSpecs( int attrib ) const
						{ return 0; }

    virtual bool		canHaveMultipleTextures() const { return false;}
    virtual int			nrTextures(int attrib) const	{ return 0; }
    virtual void		selectTexture(int attrib,int texture) {}
    virtual int			selectedTexture(int attrib) const { return 0; }
    virtual void		allowShading(bool)		{}
    virtual bool		getSelMousePosInfo(const visBase::EventInfo&,
						   Coord3&, BufferString&,
						   BufferString&) const
				{ return false; }
    virtual void		getMousePosInfo(const visBase::EventInfo&,
					    Coord3& xyzpos,
					    BufferString& val,
					    BufferString& info) const
				{ val = mUdf(float); info = ""; }
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const;
    virtual const MouseCursor*	getMouseCursor() const		{ return 0; }

				/*!<Returns a mouse cursor that will
				    be used if this object under the
				    mouse in Act mode. */
    virtual void		updateMouseCursorCB(CallBacker*)	{}

    virtual void		getObjectInfo(BufferString&) const	{}

				// Data via DataPacks
    virtual bool		setDataPackID(int attrib,DataPack::ID,
					      TaskRunner*)
				{ return false; }
    virtual DataPack::ID	getDataPackID(int attrib) const
				{ return DataPack::ID::getInvalid(); }
    virtual DataPack::ID	getDisplayedDataPackID(int attrib) const
				{ return DataPack::ID::getInvalid(); }
    virtual DataPackMgr::ID	getDataPackMgrID() const
				{ return DataPackMgr::ID::getInvalid(); }

				//Volume data
    virtual TrcKeyZSampling	getTrcKeyZSampling( int attrib ) const
				{ TrcKeyZSampling cs; return cs; }
				/*!<\returns the volume in world survey
				     coordinates. */
    virtual bool		setDataVolume(int attrib,
					      const RegularSeisDataPack*,
					      TaskRunner*)
				{ return false; }
    virtual const RegularSeisDataPack* getCacheVolume(int attr) const
				{ return 0; }

				//Trace-data
    virtual void		getTraceKeyPath(TrcKeyPath&,
                                                TypeSet<Coord>* = 0) const {}
                                /*!<If Coordinates are different from the
                                   trckeys, they can be delivered in the
                                   TypeSet<Coord>* */
    virtual void		getDataTraceBids(TypeSet<BinID>&) const	{}
    virtual Interval<float>	getDataTraceRange() const
				{ return Interval<float>(0,0); }

				/*! Random pos: Every position is put in the
				  DataPointSet no matter which original patch
				  it belongs to*/
    virtual void		getRandomPos(DataPointSet&,TaskRunner*) const {}
    virtual void		getRandomPosCache(int attrib,
						  DataPointSet&) const	{}
    virtual void		setRandomPosData(int attrib,const DataPointSet*,
					 const TaskRunnerProvider&)	{}
    virtual void		readAuxData()				{}

    virtual void		setScene(Scene* scn);
    virtual const Scene*	getScene() const	{ return scene_; }
    virtual Scene*		getScene()		{ return scene_; }
    virtual int			getSceneID() const	{ return scene_->id(); }

    virtual bool		setZAxisTransform(ZAxisTransform*,TaskRunner*)
				{return false;}
    virtual const ZAxisTransform* getZAxisTransform() const	 {return 0;}
    virtual bool		alreadyTransformed(int attrib) const;

    virtual void		annotateNextUpdateStage(bool yn);
				/*!<Annotation to enumerate distinguishable
				    stages in an update sequence. False resets
				    updatestagenr_ to zero. For example:
				    <code>
					// no updating		#0
					annotateNextUpdateStage( true );
					// update geometry	#1
					annotateNextUpdateStage( true );
					// update textures	#2
					annotateNextUpdateStage( false );
					// no updating		#0
				    </code>
				    Derived class decides whether to neglect or
				    act, e.g. by (de/re)freezing its display. */
    int				getUpdateStageNr() const;

    virtual void		lock( bool yn )		{ locked_ = yn; }
    virtual bool		isLocked() const	{ return locked_; }
    virtual NotifierAccess*	getLockNotifier()	{ return 0; }
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual bool		canBDispOn2DViewer() const	{ return false;}
    virtual bool		isVerticalPlane() const		{ return true;}
    virtual bool		isInlCrl() const		{ return false;}
    virtual void		turnOnSelectionMode(bool) {};
    virtual bool		isSection() const		{ return false;}

    static float		sDefMaxDist();

				//Old
    static const char*		sKeyColTabID()	{ return "Colortable ID"; }

				//Current
    static const char*		sKeySequence()	{ return "Sequence"; }
    static const char*		sKeyMapper()	{ return "Mapper"; }
    static const char*		sKeyTextTrans()	{ return "Trans"; }
    static const char*		sKeyTC2RGBA()	{ return "TC2RGBA"; }
    static const char*		sKeyNrAttribs() { return "Nr Attribs"; }
    static const char*		sKeyAttribs()	{ return "Attrib "; }
    static const char*		sKeyLocked()	{ return "Locked"; }
    static const char*		sKeySurvey()	{ return "Survey"; }

    void			setUserRefs( int attrib, BufferStringSet* nms )
				{ delete (userrefs_.validIdx(attrib) ?
					 userrefs_.replace(attrib,nms) : nms); }

    void			setSaveInSessionsFlag( bool yn )
				{ saveinsessionsflag_ = yn; }
    bool			getSaveInSessionsFlag() const
				{ return saveinsessionsflag_; }
    virtual bool		canBeRemoved()const { return true; }

protected:
				SurveyObject();
				~SurveyObject();

    void			initAdaptiveMouseCursor(CallBacker* eventcb,
						int objid,int inplanedragkeys,
						MouseCursor&);

    static int			cValNameOffset()	{ return 12; }

    mutable uiString		errmsg_;
    Scene*			scene_;
    int				updatestagenr_;
    bool			locked_;
    ObjectSet<BufferStringSet>	userrefs_;

    const SurvGeom3D*		s3dgeom_;
    BufferString		survname_; //Only from IOPar
    bool			saveinsessionsflag_;

};

} // namespace visSurvey

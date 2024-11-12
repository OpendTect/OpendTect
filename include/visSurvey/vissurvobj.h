#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "attribsel.h"
#include "datapackbase.h"
#include "factory.h"
#include "gendefs.h"
#include "multiid.h"
#include "position.h"
#include "ranges.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "vissurvscene.h"

class DataPointSet;
class NotifierAccess;
class SeisTrcBuf;
class TaskRunner;
class uiString;
class ZAxisTransform;

namespace ColTab  { class MapperSetup; class Sequence; }
namespace OD { class LineStyle; }
class MarkerStyle3D;

namespace visBase
{
    class EventInfo;
    class Transformation;
    class TextureChannels;
    class TextureChannel2RGBA;
}

namespace Attrib { class SelSpec; }

namespace visSurvey
{

/*!
\brief Base class for all 'Display' objects
*/

mExpClass(visSurvey) SurveyObject
{
public:
				mDefineFactoryInClass(SurveyObject,factory)

    virtual void		set3DSurvGeom(const Survey::Geometry3D*);
    const Survey::Geometry3D*	get3DSurvGeom() const { return s3dgeom_.ptr(); }
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
    virtual void		getPickingMessage( BufferString& msg ) const
				{ msg = "Picking"; }

    virtual bool		isPainting() const	{ return false; }

    virtual void		snapToTracePos(Coord3&)	const {}
				//<\Snaps coordinate to a trace position

    virtual NotifierAccess*	getMovementNotifier()	{ return nullptr; }
				/*!<Gives access to a notifier that is triggered
				    when object is moved or modified. */

    virtual void		otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    const VisID& whichobj) {}
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

    virtual void		getChildren(TypeSet<VisID>&) const	{}

    virtual bool		canDuplicate() const	{ return false;}
    virtual SurveyObject*	duplicate(TaskRunner*) const	{ return 0; }

    virtual MultiID		getMultiID() const { return MultiID::udf(); }

    virtual bool		hasPosModeManipulator() const	{ return false;}
    virtual void		showManipulator(bool yn)	{}
    virtual bool		isManipulatorShown() const	{ return false;}
    virtual bool		isManipulated() const		{ return false;}
    virtual bool		canResetManipulation() const	{ return false;}
    virtual void		resetManipulation()		{}
    virtual void		acceptManipulation()		{}
    virtual uiString		getManipulationString() const
						{ return uiString::empty(); }
    virtual NotifierAccess*	getManipulationNotifier()   { return nullptr; }
    virtual void		enableEditor(bool yn)		{}

    virtual bool		allowMaterialEdit() const   { return false;}
				/*!\note Modification of color should be done
					 with setMaterial on
					 visBase::VisualObject */

    virtual const MarkerStyle3D* markerStyle() const	{ return nullptr; }
    virtual void		setMarkerStyle(const MarkerStyle3D&)	{}
    virtual bool		hasSpecificMarkerColor() const { return false; }

    virtual const OD::LineStyle* lineStyle() const	{ return nullptr; }
				/*!<If the linestyle can be set, a non-zero
				    pointer should be return. */
    virtual void		setLineStyle(const OD::LineStyle&)	{}
    virtual void		getLineWidthBounds(int& min,int& max);
    virtual bool		hasSpecificLineColor() const { return false; }
				/*!<Specifies wether setLineStyle takes
				    regard to the color of the linestyle. */
    virtual void		setOnlyAtSectionsDisplay(bool)		{}
    virtual bool		displayedOnlyAtSections() const { return false;}

    virtual bool		hasColor() const	{ return false;}
    virtual bool		usesColor() const	{ return hasColor(); }
    virtual void		setColor(OD::Color)		{}
    virtual OD::Color		getColor() const
				{ return OD::Color::DgbColor(); }

    OD::Color			getBackgroundColor() const;
    virtual void		setAnnotColor(OD::Color)		{}
    virtual OD::Color		getAnnotColor() const
				{ return OD::Color::DgbColor(); }

    virtual void		useTexture(bool yn,bool trigger)	{}
    virtual bool		usesTexture() const;
    virtual bool		showsTexture() const;
    virtual bool		canShowTexture() const		{ return false;}

    virtual int			nrResolutions() const		{ return 1; }
    virtual BufferString	getResolutionName(int) const;
    virtual int			getResolution() const		{ return 0; }
    virtual void		setResolution(int,TaskRunner*)	{}

    virtual visBase::TextureChannels* getChannels()	     { return nullptr; }
    virtual const visBase::TextureChannels* getChannels() const
							     { return nullptr; }
    virtual visBase::TextureChannel2RGBA* getChannels2RGBA() { return nullptr; }
    virtual const visBase::TextureChannel2RGBA*   getChannels2RGBA() const;
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
    virtual int			maxNrAttribs() const		{ return 1; }
    virtual bool		canAddAttrib(int nrattribstoadd=1) const;
    virtual bool		addAttrib()		   { return false; }
    virtual bool		canRemoveAttrib() const;
    virtual bool		removeAttrib(int attrib)   { return false; }
    virtual bool		swapAttribs(int a0,int a1) { return false; }
    virtual void		setAttribTransparency(int,unsigned char) {}
    virtual unsigned char	getAttribTransparency(int) const { return 0; }
    virtual const ColTab::MapperSetup*	getColTabMapperSetup(int attrib,
							   int version=0) const;
    void			getChannelName(int,uiString&) const;
				//!<\returns "Layer 0", or "Red", "Green" ...
    virtual void		setColTabMapperSetup(int,
				     const ColTab::MapperSetup&,TaskRunner*);
    virtual const ColTab::Sequence* getColTabSequence(int) const { return 0; }
    virtual bool		canSetColTabSequence() const	{ return false;}
    virtual void		setColTabSequence(int,const ColTab::Sequence&,
						  TaskRunner*);
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

    virtual const TypeSet<float>* getHistogram(int attrib) const { return 0; }

    virtual bool		canRemoveSelection() const	{ return false;}
    virtual bool		removeSelections(TaskRunner*)	{ return false;}
    virtual void		clearSelections()		{}

    virtual void		setSelSpec(int,const Attrib::SelSpec&);
    virtual void		setSelSpecs(int attrib,
					    const TypeSet<Attrib::SelSpec>&);

    virtual const Attrib::SelSpec* getSelSpec(int attrib,int version=-1) const;
				   //!< version=-1 gives current version

    virtual const TypeSet<Attrib::SelSpec>* getSelSpecs( int attrib ) const
					    { return nullptr; }
    bool			hasSelSpec(const Attrib::SelSpec&,int& attrib,
					   int& version) const;

    virtual bool		canHaveMultipleTextures() const { return false;}
    virtual int			nrTextures(int attrib) const	{ return 0; }
    virtual void		selectTexture(int attrib,int texture) {}
    virtual int			selectedTexture(int attrib) const { return 0; }
    virtual void		allowShading(bool)		{}
    virtual void		getMousePosInfo(const visBase::EventInfo&,
					    Coord3& xyzpos,
					    BufferString& val,
					    uiString& info) const
				{ val = mUdf(float); info.setEmpty(); }
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const;
    virtual const MouseCursor*	getMouseCursor() const	{ return nullptr; }

				/*!<Returns a mouse cursor that will
				    be used if this object under the
				    mouse in Act mode. */
    virtual void		updateMouseCursorCB(CallBacker*)	{}

    virtual void		getObjectInfo(uiString&) const	{}

				// Data via DataPacks
    virtual bool		usesDataPacks() const	{ return false; }
    virtual ConstRefMan<DataPack> getDataPack(int attrib) const
				{ return nullptr; }
    virtual ConstRefMan<DataPack> getDisplayedDataPack(int attrib) const;

				//PointSet-based datapacks
    virtual bool		setPointDataPack(int attr,PointDataPack*,
						 TaskRunner*)
				{ return false; }
    virtual ConstRefMan<PointDataPack> getPointDataPack(int attr) const
				{ return nullptr; }

				//Flat datapacks
    virtual bool		setFlatDataPack(int attr,FlatDataPack*,
						TaskRunner*)
				{ return false; }
    virtual ConstRefMan<FlatDataPack> getFlatDataPack(int attr) const
				{ return nullptr; }

				//Volume data
    virtual TrcKeyZSampling	getTrcKeyZSampling(bool displayspace=false,
						int attrib=-1) const;
				/*!<\returns the volume in world survey
				     coordinates. */
    virtual bool		setVolumeDataPack(int attrib,VolumeDataPack*,
						  TaskRunner*)
				{ return false; }
    virtual ConstRefMan<VolumeDataPack> getVolumeDataPack(int attr) const
				{ return nullptr; }
    virtual ConstRefMan<VolumeDataPack>
				getDisplayedVolumeDataPack(int attr) const;

				//Trace-data
    virtual void		getTraceKeyPath(TrcKeyPath&,
						TypeSet<Coord>* = nullptr) const
				{}
				/*!<If Coordinates are different from the
				    trckeys, they can be delivered in the
				    TypeSet<Coord>* */
    virtual void		getDataTraceBids(TypeSet<BinID>&) const	{}
    virtual Interval<float>	getDataTraceRange() const
				{ return Interval<float>(0.f,0.f); }

				/*! Random pos: Every position is put in the
				  DataPointSet no matter which original patch
				  it belongs to*/
    virtual bool		getRandomPos(DataPointSet&,TaskRunner*) const
				{ return false; }
    virtual bool		getRandomPosCache(int attrib,
						  DataPointSet&) const
				{ return false; }
    virtual bool		setRandomPosData(int attrib,const DataPointSet*,
						 TaskRunner*) { return false; }
    virtual void		readAuxData()				{}

    virtual void		setScene(Scene*);
    virtual const Scene*	getScene() const	{ return scene_; }
    virtual Scene*		getScene()		{ return scene_; }
    SceneID			getSceneID() const;

    virtual bool		setZAxisTransform(ZAxisTransform*,TaskRunner*)
				{ return false; }
    virtual const ZAxisTransform* getZAxisTransform() const
				{ return nullptr; }
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
    virtual NotifierAccess*	getLockNotifier()	{ return nullptr; }
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    virtual bool		canBDispOn2DViewer() const	{ return false;}
    virtual bool		isVerticalPlane() const		{ return true;}
    virtual bool		isInlCrl() const		{ return false;}
    virtual void		turnOnSelectionMode(bool) {}

    static float		sDefMaxDist();

				//Old
    static const char*		sKeyColTabID()	{ return "Colortable ID"; }

				//Current
    static const char*		sKeySequence()	{ return "Sequence"; }
    static const char*		sKeyMapper()	{ return "Mapper"; }
    static const char*		sKeyTextTrans()	{ return "Trans"; }
    static const char*		sKeyTC2RGBA()	{ return "TC2RGBA"; }
    static const char*		sKeyNrAttribs() { return "Nr Attribs"; }
    static const char*		sKeyNrVersions(){ return "Nr Versions"; }
    static const char*		sKeySelTexture(){ return "Selected Texture"; }
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

    virtual bool		canBeRemoved() const { return true; }

protected:
				SurveyObject();
    virtual			~SurveyObject();

    void			initAdaptiveMouseCursor(CallBacker* eventcb,
						const VisID&,
						int inplanedragkeys,
						MouseCursor&);

    static int			cValNameOffset()	{ return 12; }

    mutable BufferString	errmsg_;
    Scene*			scene_			= nullptr;
    int				updatestagenr_		= 0;
    bool			locked_			= false;
    ObjectSet<BufferStringSet>	userrefs_;

    ConstRefMan<Survey::Geometry3D> s3dgeom_;
    BufferString		survname_; //Only from IOPar
    bool			saveinsessionsflag_	= true;

    bool			usestexture_		= true;
    bool			validtexture_		= false;

    friend class Scene;

public:

    mDeprecated("Provide the datapack directly")
    virtual bool		setDataPackID(int attrib,const DataPackID&,
					      TaskRunner*);
    mDeprecated("Retrieve the datapack directly")
    virtual DataPackID		getDataPackID(int attrib) const;
    mDeprecated("Retrieve the datapack directly")
    virtual DataPackID		getDisplayedDataPackID(int attrib) const;
    mDeprecatedObs
    virtual DataPackMgr::MgrID	getDataPackMgrID() const
				{ return DataPack::MgrID::udf(); }

};

} // namespace visSurvey

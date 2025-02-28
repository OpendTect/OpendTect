#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"

#include "uiapplserv.h"
#include "uimenuhandler.h"
#include "uivispickretriever.h"

#include "datapackbase.h"
#include "flatview.h"
#include "keyboardevent.h"
#include "menuhandler.h"
#include "mouseevent.h"
#include "ranges.h"
#include "trckeyzsampling.h"
#include "thread.h"
#include "vispolygonselection.h"
#include "vissurvscene.h"

#include <typeinfo>

class BufferStringSet;
class DataPointSet;
class MouseCursorExchange;
class PickSet;
class RandomSeisDataPack;
class RegularSeisDataPack;
class SeisTrcBuf;
class SurfaceInfo;
class TaskRunner;
class ZAxisTransform;
class uiMPEMan;
class uiMultiMapperRangeEditWin;
class uiSlicePos3DDisp;
class uiSurvTopBotImageDlg;
class uiToolBar;
class uiVisModeMgr;
class uiDirLightDlg;
template <class T> class Selector;

namespace Attrib    { class SelSpec; }
namespace FlatView  { class DataDispPars; }
namespace Threads   { class Mutex; }
namespace visBase   { class DataObject; class EventCatcher; }
namespace visSurvey { class Scene; }
namespace ColTab    { class Sequence; class MapperSetup; }
namespace ZDomain   { class Info; }


/*!
\brief The Visualization Part Server
*/

mExpClass(uiVis) uiVisPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiVisPartServer)
    friend class	uiMenuHandler;
    friend class	uiVisModeMgr;

public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();

    const char*		name() const override;
			/*<\returns the partservers name */
    NotifierAccess&	removeAllNotifier();
			/*<\Returns a notifier that is triggered
				    when the entire visualization is
				    closed. All visBase::DataObjects
				    must then be unrefed.  */

    void		setMouseCursorExchange(MouseCursorExchange*);

    const visBase::DataObject* getObject(const VisID&) const;
    visBase::DataObject* getObject(const VisID&);
    VisID		highestID() const;
    void		addObject(visBase::DataObject*,const SceneID&,
				  bool saveinsessions);
    void		shareObject(const SceneID&,const VisID&);
    void		findObject(const std::type_info&,TypeSet<VisID>&) const;
    void		findObject(const MultiID&,TypeSet<VisID>&) const;
    void		removeObject(visBase::DataObject*,const SceneID&);
    void		removeObject(const VisID&,const SceneID&);
    void		setSceneName(const SceneID&,const uiString&);
    void		setUiObjectName(const VisID&,const uiString&);
    void		setObjectName(const VisID&,const char*);
    uiString		getSceneName(const SceneID&) const;
    uiString		getUiObjectName(const VisID&) const;
    Pos::GeomID		getGeomID(const VisID&) const;

    CNotifier<uiVisPartServer,VisID>	objectAdded;
    CNotifier<uiVisPartServer,VisID>	objectRemoved;

    void		removeSelection();

    SceneID		addScene(visSurvey::Scene* =nullptr);
			/*!<Adds a scene. The argument is only used internally.
			    Don't use the argument when calling from outside.
			*/
    void		removeScene(const SceneID&);
    int			nrScenes() const;
    NotifierAccess&	nrScenesChange() { return nrscenesChange; }

    void		getSceneIds(TypeSet<SceneID>&) const;
    visSurvey::Scene*	getScene(const SceneID&);
    const visSurvey::Scene* getScene(const SceneID&) const;

    void		getSceneChildIds(const SceneID&,TypeSet<VisID>&) const;
    void		getVisChildIds(const VisID&,TypeSet<VisID>&) const;

    bool		clickablesInScene(const char* trackertype,
					  const SceneID&) const;
    bool		getClickableAttributesInScene(TypeSet<Attrib::SelSpec>&,
						      BufferStringSet& attrnms,
						      const char* trackertype,
						      const SceneID&) const;

    bool		hasAttrib(const VisID&) const;
    enum AttribFormat	{ None, Cube, Traces, RandomPos, OtherFormat };
			/*!\enum AttribFormat
				 Specifies how the object wants it's
				 attrib data delivered.
			   \var None
				This object does not handle attribdata.
			   \var	Cube
				This object wants attribdata as DataCubes.
			   \var Traces
				This object wants a set of traces.
			   \var RandomPos
				This object wants a table with
				array positions.
			   \var	OtherFormat
				This object wants data in a different format. */

    AttribFormat	getAttributeFormat(const VisID&,int attrib) const;
    bool		canHaveMultipleAttribs(const VisID&) const;
    bool		canAddAttrib(const VisID&,int nrattribstoadd=1) const;
    bool		canRemoveAttrib(const VisID&) const;
    bool		canRemoveDisplay(const VisID&) const;
    int			addAttrib(const VisID&);
    void		removeAttrib(const VisID&,int attrib);
    int			getNrAttribs(const VisID&) const;
    void		getAttribPosName(const VisID&,int attrib,
					 uiString&) const;
			//!<Gets the name of the attrib position
    bool		swapAttribs(const VisID&,int attrib0,int attrib1);
    void		showAttribTransparencyDlg(const VisID&,int attrib);
    unsigned char	getAttribTransparency(const VisID&,int attrib) const;
    void		setAttribTransparency(const VisID&,int attrib,
					      unsigned char);
    const TypeSet<Attrib::SelSpec>* getSelSpecs(const VisID&,int attrib) const;
    const Attrib::SelSpec* getSelSpec(const VisID&,int attrib) const;

    void		setSelSpec(const VisID&,int attrib,
				   const Attrib::SelSpec&);
    void		setSelSpecs(const VisID&,int attrib,
				    const TypeSet<Attrib::SelSpec>&);
    bool		selectAttribForTracking();
    void		setUserRefs(const VisID&,int attrib,BufferStringSet*);
    bool		interpolationEnabled(const VisID&) const;
			/*!<Specifies that the data is integers that should
			    be interpolated. */
    void		enableInterpolation(const VisID&,bool yn);
			/*!<Specify that the data is integers that should
			    be interpolated. */
    bool		isAngle(const VisID&,int attrib) const;
			/*!<Specifies that the data is angles, i.e. -PI==PI. */
    void		setAngleFlag(const VisID&,int attrib,bool yn);
			/*!<Specify that the data is angles, i.e. -PI==PI. */
    bool		isAttribEnabled(const VisID&,int attrib) const;
    void		enableAttrib(const VisID&,int attrib,bool yn);
    bool		hasSingleColorFallback(const VisID&) const;
    void		setTranslation(const VisID&,const Coord3& shift);
    Coord3		getTranslation(const VisID&) const;

    TrcKeyZSampling	getTrcKeyZSampling(const VisID&,int attrib=-1) const;
    int			currentVersion(const VisID&,int attrib) const;

			//General datapack retrieval
    ConstRefMan<DataPack> getDataPack(const VisID&,int attrib) const;
    ConstRefMan<DataPack> getDisplayedDataPack(const VisID&,int attrib) const;

			//Volume data stuff
    bool		setRegularSeisDataPack(const VisID&,int attrib,
					       RegularSeisDataPack*);
    bool		setRandomSeisDataPack(const VisID&,int attrib,
					      RandomSeisDataPack*);
    ConstRefMan<FlatDataPack> getFlatDataPack(const VisID&,int attrib) const;
    ConstRefMan<VolumeDataPack> getVolumeDataPack(const VisID&,
						  int attrib) const;
    ConstRefMan<VolumeDataPack> getDisplayedVolumeDataPack(const VisID&,
							   int attrib) const;

			//Trace data
    void		getDataTraceBids(const VisID&,TypeSet<BinID>&) const;
    Interval<float>	getDataTraceRange(const VisID&) const;

			// See visSurvey::SurfaceDisplay for details
    bool		setPointDataPack(const VisID&,int attrib,
					 PointDataPack*);
    bool		setRandomPosData(const VisID&,int attrib,
					 const DataPointSet*);
    ConstRefMan<PointDataPack> getPointDataPack(const VisID&,
						int attrib) const;
    bool		getRandomPos(const VisID&,DataPointSet&) const;
    bool		getRandomPosCache(const VisID&,int attrib,
					  DataPointSet&) const;

    bool		hasMaterial(const VisID&) const;
    void		setMaterial(const VisID&);
    bool		hasColor(const VisID&) const;
    void		setColor(const VisID&,const OD::Color&);

    bool		blockMouseSelection(bool yn);
			/*!<\returns Previous status. */

    bool		disabMenus(bool yn);
			/*!<\returns The previous status. */
    void		createToolBars();
    bool		disabToolBars(bool yn);
			/*!<\returns The previous status. */

    bool		showMenu(const VisID&,int menutype=0,
				 const TypeSet<int>* =nullptr,
				 const Coord3& = Coord3::udf());
			/*!<
			  \param menuid
			  \param menutype Please refer to \ref
				uiMenuHandler::executeMenu for a detailed
				description.
			*/

    MenuHandler*	getMenuHandler();
    MenuHandler*	getToolBarHandler();

    MultiID		getMultiID(const VisID&) const;

    VisID		getSelObjectId() const;
    int			getSelAttribNr() const;
    void		setSelObjectId(const VisID&,int attrib=-1);
    void		setCurInterObjID(const VisID&);
    VisID		getCurInterObjID() const;
    SceneID		getSceneID(const VisID&) const;
    const ZDomain::Info* zDomainInfo(const SceneID&) const;
			/*!< Returns Z domain info of scene */

			//Events and their functions
    void		unlockEvent();
			/*!< This function _must_ be called after
			     the object has sent an event to unlock
			     the object. */
    VisID		getEventObjId() const;
			/*<\returns the id that triggered the event */
    int			getEventAttrib() const;
			/*<\returns the attrib that triggered the event */

    static int		evUpdateTree();
    void		triggerTreeUpdate();

    static int		evSelection();
			/*<! Get the id with getEventObjId() */

    static int		evDeSelection();
			/*<! Get the id with getEventObjId() */

    static int		evGetNewData();
			/*!< Get the id with getEventObjId() */
			/*!< Get the attrib with getEventAttrib() */
			/*!< Get selSpec with getSelSpec */

    void		calculateAllAttribs();
    void		calculateAllAttribs(const VisID&);
    bool		calculateAttrib(const VisID&,int attrib,bool newsel,
					bool ignorelocked=false);
    bool		calcManipulatedAttribs(const VisID&);

    void		movePlaneAndCalcAttribs(const VisID&,
						const TrcKeyZSampling&);

    bool		canHaveMultipleTextures(const VisID&) const;
    int			nrTextures(const VisID&,int attrib) const;
    void		selectTexture(const VisID&,int attrib,int texture);
    int			selectedTexture(const VisID&,int attrib) const;

    static int		evMouseMove();
    SceneID		getMouseSceneID() const		{ return mousescene_; }
    Coord3		getMousePos() const		{ return xytmousepos_; }
    BufferString	getMousePosVal() const		{ return mouseposval_; }
    BufferString	getMousePosString() const	{ return mouseposstr_; }
    void		getObjectInfo(const VisID&,uiString&) const;

    static int			evKeyboardEvent();
    Notifier<uiVisPartServer>	keyEvent;
    const KeyboardEvent&	getKeyboardEvent() const { return kbevent_; }
    static int			evMouseEvent();
    Notifier<uiVisPartServer>	mouseEvent;
    const MouseEvent&		getMouseEvent() const	{ return mouseevent_; }
    void			setSceneEventHandled();

    static int			evSelectAttrib();

    static int			evInteraction();
				/*<! Get the id with getEventObjId() */
    uiString			getInteractionMsg(const VisID&) const;
				/*!< Returns dragger position or
				     Nr positions in picksets */

    static int			evViewAll();
    static int			evToHomePos();

				// ColorTable stuff
    mDeprecated("Use method that takes FlatView::Viewer::VwrDest enum")
    void			fillDispPars(const VisID&,int attrib,
					 FlatView::DataDispPars&,bool) const;
    void			fillDispPars(const VisID&,int attrib,
					     FlatView::DataDispPars&,
					     FlatView::Viewer::VwrDest) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(const VisID&,int attrib,
						 int version=mUdf(int)) const;
    void			setColTabMapperSetup(const VisID&,int attrib,
						    const ColTab::MapperSetup&);
    const ColTab::Sequence*	getColTabSequence(const VisID&,
						  int attrib) const;
    bool			canSetColTabSequence(const VisID&) const;
    void			setColTabSequence(const VisID&,int attrib,
						  const ColTab::Sequence&);
    bool			canHandleColTabSeqTrans(const VisID&,
							int attr) const;

    const TypeSet<float>*	getHistogram(const VisID&,int attrib) const;

    void			displayMapperRangeEditForAttribs(const VisID&,
								 int attrib=-1);
				//!< attrib=-1 for all

    static int			evColorTableChange();
    void			displaySceneColorbar(bool);
    void			manageSceneColorbar(const SceneID&);
    bool			sceneColorbarDisplayed();

    OD::Color			getSceneAnnotCol(int);

				//General stuff
    bool			deleteAllObjects();
    void			setZStretch();
    void			setDirectionalLight();
    bool			setWorkingArea();
    bool			setWorkingArea(const TrcKeyZSampling&);
    bool			setWorkingArea(const SceneID&);
    void			setOnlyAtSectionsDisplay(const VisID&,bool);
    bool			displayedOnlyAtSections(const VisID&) const;
    static int			evViewModeChange();
    void			setViewMode(bool yn,bool notify=true);
    void			setSoloMode(bool,TypeSet< TypeSet<VisID> >,
					    const VisID&);
    bool			isSoloMode() const;
    bool			isViewMode() const;
    typedef enum		{ View, Interactive, Pick } WorkMode;
    void			setWorkMode(WorkMode,bool notify=true);
    WorkMode			getWorkMode() const;
    enum			SelectionMode { Polygon, Rectangle };
    void			setSelectionMode(SelectionMode);
    SelectionMode		getSelectionMode() const;
    void			turnSelectionModeOn(bool);
    bool			isSelectionModeOn() const;
    Notifier<uiVisPartServer>	selectionmodeChange;
    void			setZAxisTransform(const SceneID&,
						  ZAxisTransform*,
						  TaskRunner*);
    const ZAxisTransform*	getZAxisTransform(const SceneID&) const;
    visBase::EventCatcher*	getEventCatcher(const SceneID&);

    const Selector<Coord3>*	getCoordSelector(const SceneID&) const;
    void			turnOn(const VisID&,bool,bool doclean=false);
    bool			isOn(const VisID&) const;
    void			updateDisplay(bool,const VisID& selid,
					      const VisID& refid=VisID::udf());
    void			setTopBotImg(const SceneID&);

    bool			canDuplicate(const VisID&) const;
    VisID			duplicateObject(const VisID&,const SceneID&);
				/*!< \returns id of new object */

				// Tracking stuff
    void			turnSeedPickingOn(bool yn);
    static int			evPickingStatusChange();
    static int			evDisableSelTracker();
    static int			evShowMPESetupDlg();
    static int			evShowMPEParentPath();

    void			reportTrackingSetupActive(bool yn);
    bool			isTrackingSetupActive() const;

    bool			isPicking() const;
				/*!<\returns true if the selected object
				     is handling left-mouse picks on other
				     objects, so the picks won't be handled by
				     the selman. */
    void			getPickingMessage(BufferString&) const;

    static int			evShowSetupGroupOnTop();
    bool			showSetupGroupOnTop(const char* grpnm);
    const char*			getTopSetupGroupName() const;

    void			initMPEStuff();
    static int			evStoreEMObject();
    static int			evStoreEMObjectAs();
    void			storeEMObject(bool storeas);

    uiSlicePos3DDisp*		getUiSlicePos() const
				{ return slicepostools_; }

    bool			writeSceneToFile(const SceneID&,
						const uiString& dlgtitle) const;

    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    bool			canBDispOn2DViewer(const VisID&) const;
    bool			isVerticalDisp(const VisID&) const;

    void			lock(const VisID&,bool yn);
    bool			isLocked(const VisID&) const;

    bool			sendVisEvent(int);
    void			setMoreObjectsToDoHint(const SceneID&,bool yn);
    bool			getMoreObjectsToDoHint(const SceneID&) const;
    Notifier<uiVisPartServer>	planeMovedEvent;

protected:

    void			createMenuCB(CallBacker*);
    void			addToToolBarCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    bool			selectAttrib(const VisID&,int attrib);
    void			updateManipulatorStatus(visBase::DataObject*,
							bool issel) const;

    bool			setVolumeDataPack(const VisID&,int attrib,
						  VolumeDataPack*);
    void			setMarkerPos(const TrcKeyValue&,
					     const SceneID& dontsetscene);

    bool			isManipulated(const VisID&) const;
    void			acceptManipulation(const VisID&);
    bool			resetManipulation(const VisID&);

    void			setUpConnections(const VisID&);
				/*!< Should set all cbs for the object */
    void			removeConnections(const VisID&);

    void			updateDraggers();
    int				getTypeSetIdx(const VisID&);

    RefObjectSet<visSurvey::Scene> scenes_;

    RefMan<uiMenuHandler>	menu_;
    RefMan<uiTreeItemTBHandler> toolbar_;

    uiMPEMan*			mpetools_			= nullptr;
    uiSlicePos3DDisp*		slicepostools_			= nullptr;
    uiSurvTopBotImageDlg*	topbotdlg_			= nullptr;

    uiMultiMapperRangeEditWin*	multirgeditwin_			= nullptr;
    bool			mapperrgeditinact_		= false;
    VisID			mapperrgeditordisplayid_;

    Coord3			xytmousepos_			= Coord3::udf();
    SceneID			mousescene_;
    BufferString		mouseposval_;
    BufferString		mouseposstr_;
    KeyboardEvent		kbevent_;
    MouseEvent			mouseevent_;
    RefMan<visSurvey::Scene>	sceneeventsrc_;

    bool			tracksetupactive_		= false;
    const char*			topsetupgroupname_		= nullptr;
    bool			viewmode_			= false;
    WorkMode			workmode_	= uiVisPartServer::Interactive;
    bool			issolomode_			= false;
    Threads::Mutex&		eventmutex_;
    VisID			eventobjid_			= VisID::udf();
    int				eventattrib_			= -1;
    int				selattrib_			= -1;
    VisID			curinterpobjid_;

    visBase::PolygonSelection::SelectionType seltype_
					= visBase::PolygonSelection::Off;
    SelectionMode		selectionmode_			= Polygon;

    void			mouseCursorCB(CallBacker*);
    void			rightClickCB(CallBacker*);
    void			selectObjCB(CallBacker*);
    void			deselectObjCB(CallBacker*);
    void			updateSelObjCB(CallBacker*);
    void			datasetUpdatedCB(CallBacker*);
    void			interactionCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			keyEventCB(CallBacker*);
    void			mouseEventCB(CallBacker*);
    void			vwAll(CallBacker*);
    void			toHome(CallBacker*);
    void			colTabChangeCB(CallBacker*);

    void			mapperRangeEditChanged(CallBacker*);
    void			sequenceEditChanged(CallBacker*);

    MenuItem			resetmanipmnuitem_;
    MenuItem			changematerialmnuitem_;
    MenuItem			resmnuitem_;

    TypeSet< TypeSet<VisID> >	displayids_;

    static const char*		sKeyWorkArea();
    static const char*		sKeyAppVel();

    uiVisModeMgr*		vismgr_;
    bool			blockmenus_		= false;
    RefMan<uiVisPickRetriever>	pickretriever_;
    Notifier<uiVisPartServer>	nrscenesChange;

    MouseCursorExchange*	mousecursorexchange_	= nullptr;

    uiDirLightDlg*		dirlightdlg_		= nullptr;

    void			triggerObjectMoved(const VisID&);

public:
    mDeprecated("Provide the datapack directly")
    bool		setDataPackID(const VisID&,int attrib,
				      const DataPackID&);
    mDeprecated("Retrieve the datapack directly")
    DataPackID		getDataPackID(const VisID&,int attrib) const;
    mDeprecated("Retrieve the datapack directly")
    DataPackID		getDisplayedDataPackID(const VisID&,int attrib) const;
    mDeprecatedObs
    DataPackMgr::MgrID	getDataPackMgrID(const VisID&) const;
};


mClass(uiVis) uiVisModeMgr
{
public:
				uiVisModeMgr(uiVisPartServer*);
				~uiVisModeMgr();

	bool			allowTurnOn(const VisID&,bool);

protected:

	uiVisPartServer&	visserv;
};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uiapplserv.h"

#include "trckeyzsampling.h"
#include "datapack.h"
#include "flatview.h"
#include "keyboardevent.h"
#include "menuhandler.h"
#include "mouseevent.h"
#include "ranges.h"
#include "thread.h"
#include <typeinfo>

class BufferStringSet;
class DataPointSet;
class MouseCursorExchange;
class PickSet;
class RegularSeisDataPack;
class SeisTrcBuf;
class SurfaceInfo;
class TaskRunner;
class ZAxisTransform;
class uiMenuHandler;
class uiMPEMan;
class uiMultiMapperRangeEditWin;
class uiSlicePos3DDisp;
class uiSurvTopBotImageDlg;
class uiToolBar;
class uiTreeItemTBHandler;
class uiVisModeMgr;
class uiVisPickRetriever;
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

    const char*		name() const;
			/*<\returns the partservers name */
    NotifierAccess&	removeAllNotifier();
			/*<\Returns a notifier that is triggered
				    when the entire visualization is
				    closed. All visBase::DataObjects
				    must then be unrefed.  */

    void		setMouseCursorExchange(MouseCursorExchange*);

    visBase::DataObject* getObject(VisID) const;
    VisID		highestID() const;
    void		addObject(visBase::DataObject*,SceneID sceneid,
				  bool saveinsessions);
    void		shareObject(SceneID sceneid,VisID);
    void		findObject(const std::type_info&,TypeSet<VisID>&);
    void		findObject(const MultiID&, TypeSet<VisID>& );
    void		removeObject(visBase::DataObject*,SceneID sceneid);
    void		removeObject(VisID,SceneID sceneid);
    void		setUiObjectName(VisID,const uiString&);
    void		setObjectName(VisID,const char*);
    uiString		getUiObjectName(VisID) const;
    Pos::GeomID		getGeomID(VisID) const;

    CNotifier<uiVisPartServer,VisID>	objectAdded;
    CNotifier<uiVisPartServer,VisID>	objectRemoved;

    void		removeSelection();

    VisID		addScene(visSurvey::Scene* =0);
			/*!<Adds a scene. The argument is only used internally.
			    Don't use the argument when calling from outside.
			*/
    void		removeScene(VisID);
    NotifierAccess&	nrScenesChange() { return nrscenesChange; }
    bool		clickablesInScene(const char* trackertype,
					  SceneID sceneid) const;
    bool		getClickableAttributesInScene(TypeSet<Attrib::SelSpec>&,
						      BufferStringSet& attrnms,
						      const char* trackertype,
						      SceneID) const;
    const ObjectSet<visSurvey::Scene>& getAllScenes() const { return scenes_; }
    void		getSceneIds(TypeSet<SceneID>& sceneids) const;

    void		getChildIds(VisID,TypeSet<VisID>&) const;
			/*!< Gets a scenes' children or a volumes' parts
			     If id==-1, it will give the ids of the
			     scenes */

    bool		hasAttrib(VisID) const;
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

    AttribFormat	getAttributeFormat(VisID,int attrib) const;
    bool		canHaveMultipleAttribs(VisID) const;
    bool		canAddAttrib(VisID,int nrattribstoadd=1) const;
    bool		canRemoveAttrib(VisID) const;
    bool		canRemoveDisplay(VisID) const;
    int			addAttrib(VisID);
    void		removeAttrib(VisID,int attrib);
    int			getNrAttribs(VisID) const;
    void		getAttribPosName(VisID,int attrib,uiString&) const;
			//!<Gets the name of the attrib position
    bool		swapAttribs(VisID,int attrib0,int attrib1);
    void		showAttribTransparencyDlg(VisID,int attrib);
    unsigned char	getAttribTransparency(VisID,int attrib) const;
    void		setAttribTransparency(VisID,int attrib,unsigned char);
    const TypeSet<Attrib::SelSpec>* getSelSpecs(VisID,int attrib) const;
    const Attrib::SelSpec* getSelSpec(VisID,int attrib) const;

    void		setSelSpec(VisID,int attrib,const Attrib::SelSpec&);
    void		setSelSpecs(VisID,int attrib,
				    const TypeSet<Attrib::SelSpec>&);
    bool		selectAttribForTracking();
    void		setUserRefs(VisID,int attrib,BufferStringSet*);
    bool		interpolationEnabled(VisID) const;
			/*!<Specifies that the data is integers that should
			    be interpolated. */
    void		enableInterpolation(VisID,bool yn);
			/*!<Specify that the data is integers that should
			    be interpolated. */
    bool		isAngle(VisID,int attrib) const;
			/*!<Specifies that the data is angles, i.e. -PI==PI. */
    void		setAngleFlag(VisID, int attrib, bool yn);
			/*!<Specify that the data is angles, i.e. -PI==PI. */
    bool		isAttribEnabled(VisID,int attrib) const;
    void		enableAttrib(VisID,int attrib,bool yn);
    bool		hasSingleColorFallback(VisID) const;
    void		setTranslation(VisID visid,const Coord3& shift);
    Coord3		getTranslation(VisID visid) const;

			//Volume data stuff
    TrcKeyZSampling	getTrcKeyZSampling(VisID,int attrib=-1) const;
    const RegularSeisDataPack* getCachedData(VisID,int attrib) const;
    bool		setCubeData(VisID,int attrib,
				    const RegularSeisDataPack*);
			/*!< data becomes mine */
    bool		setDataPackID(VisID,int attrib,DataPackID);
    DataPackID	getDataPackID(VisID,int attrib) const;
    DataPackID	getDisplayedDataPackID(VisID,int attrib) const;
    DataPackMgr::MgrID	getDataPackMgrID(VisID) const;
    int			currentVersion(VisID,int attrib) const;

			//Trace data
    void		getDataTraceBids(VisID,TypeSet<BinID>&) const;
    Interval<float>	getDataTraceRange(VisID) const;

			// See visSurvey::SurfaceDisplay for details
    void		getRandomPos(VisID visid,DataPointSet&) const;
    void		getRandomPosCache(VisID visid,int attrib,
					  DataPointSet& ) const;
    void		setRandomPosData(VisID visid, int attrib,
					 const DataPointSet*);

    bool		hasMaterial(VisID) const;
    void		setMaterial(VisID);
    bool		hasColor(VisID) const;
    void		setColor(VisID,const OD::Color&);

    bool		blockMouseSelection(bool yn);
			/*!<\returns Previous status. */

    bool		disabMenus(bool yn);
			/*!<\returns The previous status. */
    void		createToolBars();
    bool		disabToolBars(bool yn);
			/*!<\returns The previous status. */

    bool		showMenu(VisID,int menutype=0,const TypeSet<int>* =0,
				 const Coord3& = Coord3::udf());
			/*!<
			  \param menuid
			  \param menutype Please refer to \ref
				uiMenuHandler::executeMenu for a detailed
				description.
			*/

    MenuHandler*	getMenuHandler();
    MenuHandler*	getToolBarHandler();

    MultiID		getMultiID(VisID) const;

    VisID		getSelObjectId() const;
    int			getSelAttribNr() const;
    void		setSelObjectId(VisID visid,int attrib=-1);
    void		setCurInterObjID(VisID visid);
    VisID		getCurInterObjID() const;
    SceneID		getSceneID(VisID visid) const;
    const ZDomain::Info* zDomainInfo(SceneID sceneid) const;
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
    void		calculateAllAttribs(VisID);
    bool		calculateAttrib(VisID,int attrib,bool newsel,
					bool ignorelocked=false);
    bool		calcManipulatedAttribs(VisID);

    void		movePlaneAndCalcAttribs(VisID,const TrcKeyZSampling&);

    bool		canHaveMultipleTextures(VisID) const;
    int			nrTextures(VisID,int attrib) const;
    void		selectTexture(VisID,int attrib,int texture);
    int			selectedTexture(VisID,int attrib) const;

    static int		evMouseMove();
    Coord3		getMousePos() const;
    int			zFactor() const			{ return zfactor_; }
    BufferString	getMousePosVal() const;
    BufferString	getMousePosString() const	{ return mouseposstr_; }
    void		getObjectInfo(VisID,BufferString&) const;

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
    BufferString		getInteractionMsg(VisID) const;
				/*!< Returns dragger position or
				     Nr positions in picksets */

    static int			evViewAll();
    static int			evToHomePos();

				// ColorTable stuff
    mDeprecated("Use method that takes FlatView::Viewer::VwrDest enum")
    void			fillDispPars(VisID,int attrib,
					 FlatView::DataDispPars&,bool) const;
    void			fillDispPars(VisID,int attrib,
					     FlatView::DataDispPars&,
					     FlatView::Viewer::VwrDest) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(VisID,int attrib,
						 int version=mUdf(int)) const;
    void			setColTabMapperSetup(VisID,int attrib,
						    const ColTab::MapperSetup&);
    const ColTab::Sequence*	getColTabSequence(VisID,int attrib) const;
    bool			canSetColTabSequence(VisID) const;
    void			setColTabSequence(VisID,int attrib,
						  const ColTab::Sequence&);
    bool			canHandleColTabSeqTrans(VisID,int attr) const;

    const TypeSet<float>*	getHistogram(VisID,int attrib) const;

    void			displayMapperRangeEditForAttrbs(VisID);
    void			displayMapperRangeEditForAttribs(VisID,
								 int attrib);

    static int			evColorTableChange();
    void			displaySceneColorbar(bool);
    void			manageSceneColorbar(VisID);
    bool			sceneColorbarDisplayed();

    OD::Color			getSceneAnnotCol(int);

				//General stuff
    bool			deleteAllObjects();
    void			setZStretch();
    void			setDirectionalLight();
    bool			setWorkingArea();
    bool			setWorkingArea(const TrcKeyZSampling&);
    void			setOnlyAtSectionsDisplay(VisID,bool);
    bool			displayedOnlyAtSections(VisID) const;
    static int			evViewModeChange();
    void			setViewMode(bool yn,bool notify=true);
    void			setSoloMode(bool,TypeSet< TypeSet<VisID> >,
					    VisID);
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
    void			setZAxisTransform(SceneID sceneid,
						  ZAxisTransform*,
						  TaskRunner*);
    const ZAxisTransform*	getZAxisTransform(SceneID sceneid) const;
    visBase::EventCatcher*	getEventCatcher(SceneID sceneid);

    const Selector<Coord3>*	getCoordSelector(SceneID scene) const;
    void			turnOn(VisID,bool,bool doclean=false);
    bool			isOn(VisID) const;
    void			updateDisplay(bool,VisID selid,
					      VisID refid=VisID::udf());
    void			setTopBotImg(SceneID sceneid);

    bool			canDuplicate(VisID) const;
    VisID			duplicateObject(VisID,SceneID sceneid);
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

    bool			writeSceneToFile(VisID,
						const uiString& dlgtitle) const;

    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    bool			canBDispOn2DViewer(VisID) const;
    bool			isVerticalDisp(VisID) const;

    void			lock(VisID,bool yn);
    bool			isLocked(VisID) const;

    bool			sendVisEvent(int);
    void			setMoreObjectsToDoHint(SceneID sceneid,bool yn);
    bool			getMoreObjectsToDoHint(SceneID sceneid) const;
    Notifier<uiVisPartServer>	planeMovedEvent;

protected:

    void			createMenuCB(CallBacker*);
    void			addToToolBarCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    visSurvey::Scene*		getScene(VisID);
    const visSurvey::Scene*	getScene(VisID) const;

    bool			selectAttrib(VisID, int attrib);
    void			updateManipulatorStatus(visBase::DataObject*,
							bool issel) const;

    void			setMarkerPos(const TrcKeyValue&,
					     VisID dontsetscene);

    bool			isManipulated(VisID) const;
    void			acceptManipulation(VisID);
    bool			resetManipulation(VisID);

    void			setUpConnections(VisID);
				/*!< Should set all cbs for the object */
    void			removeConnections(VisID);

    void			updateDraggers();
    int				getTypeSetIdx(VisID);

    ObjectSet<visSurvey::Scene>	scenes_;

    uiMenuHandler&		menu_;
    uiTreeItemTBHandler*	toolbar_;

    uiMPEMan*			mpetools_			= nullptr;
    uiSlicePos3DDisp*		slicepostools_			= nullptr;
    uiSurvTopBotImageDlg*	topbotdlg_			= nullptr;

    uiMultiMapperRangeEditWin*	multirgeditwin_			= nullptr;
    bool			mapperrgeditinact_;

    Coord3			xytmousepos_;
    int				zfactor_;
    BufferString		mouseposval_;
    BufferString		mouseposstr_;
    KeyboardEvent		kbevent_;
    MouseEvent			mouseevent_;
    visSurvey::Scene*		sceneeventsrc_;

    bool			tracksetupactive_;
    const char*			topsetupgroupname_;
    bool			viewmode_;
    WorkMode			workmode_;
    bool			issolomode_;
    Threads::Mutex&		eventmutex_;
    VisID			eventobjid_;
    int				eventattrib_;
    int				selattrib_;
    VisID			mapperrgeditordisplayid_;
    VisID			curinterpobjid_;

    int				seltype_;
    SelectionMode		selectionmode_;

    void			mouseCursorCB(CallBacker*);
    void			rightClickCB(CallBacker*);
    void			selectObjCB(CallBacker*);
    void			deselectObjCB(CallBacker*);
    void			updateSelObjCB(CallBacker*);
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
    bool			blockmenus_;
    uiVisPickRetriever*		pickretriever_;
    Notifier<uiVisPartServer>	nrscenesChange;

    MouseCursorExchange*	mousecursorexchange_;

    uiDirLightDlg*		dirlightdlg_;

    void			triggerObjectMoved(VisID);

public:
};


mClass(uiVis) uiVisModeMgr
{
public:
				uiVisModeMgr(uiVisPartServer*);
				~uiVisModeMgr() {}

	bool			allowTurnOn(VisID,bool);

protected:

	uiVisPartServer&	visserv;
};

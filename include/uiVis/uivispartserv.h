#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uiapplserv.h"

#include "trckeyzsampling.h"
#include "datapack.h"
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

    visBase::DataObject* getObject( int id ) const;
    int			highestID() const;
    void		addObject( visBase::DataObject*, int sceneid,
				   bool saveinsessions);
    void		shareObject(int sceneid,int id);
    void		findObject(const std::type_info&,TypeSet<int>&);
    void		findObject(const MultiID&, TypeSet<int>& );
    void		removeObject(visBase::DataObject*,int sceneid);
    void		removeObject(int id,int sceneid);
    void		setUiObjectName(int,const uiString&);
    void		setObjectName(int,const char*);
    uiString		getUiObjectName(int) const;
    Pos::GeomID		getGeomID(int) const;

    CNotifier<uiVisPartServer,int>	objectAdded;
    CNotifier<uiVisPartServer,int>	objectRemoved;

    void		removeSelection();

    int			addScene(visSurvey::Scene* =0);
			/*!<Adds a scene. The argument is only used internally.
			    Don't use the argument when calling from outside.
			*/
    void		removeScene(int);
    NotifierAccess&	nrScenesChange() { return nrscenesChange; }
    bool		clickablesInScene(const char* trackertype,
					  int sceneid) const;
    const ObjectSet<visSurvey::Scene>& getAllScenes() const { return scenes_; }
    void		getSceneIds(TypeSet<int>& sceneids) const;

    void		getChildIds(int id,TypeSet<int>&) const;
			/*!< Gets a scenes' children or a volumes' parts
			     If id==-1, it will give the ids of the
			     scenes */

    bool		hasAttrib(int) const;
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

    AttribFormat	getAttributeFormat(int id,int attrib) const;
    bool		canHaveMultipleAttribs(int id) const;
    bool		canAddAttrib(int id,int nrattribstoadd=1) const;
    bool		canRemoveAttrib(int id) const;
    bool		canRemoveDisplay(int id) const;
    int			addAttrib(int id);
    void		removeAttrib(int id,int attrib);
    int			getNrAttribs(int id) const;
    void		getAttribPosName(int id,int attrib,uiString&) const;
			//!<Gets the name of the attrib position
    bool		swapAttribs(int id,int attrib0,int attrib1);
    void		showAttribTransparencyDlg(int id,int attrib);
    unsigned char	getAttribTransparency(int id,int attrib) const;
    void		setAttribTransparency(int id,int attrib, unsigned char);
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int id,int attrib) const;
    const Attrib::SelSpec* getSelSpec(int id,int attrib) const;

    void		setSelSpec(int id,int attrib,const Attrib::SelSpec&);
    void		setSelSpecs(int id,int attrib,
				    const TypeSet<Attrib::SelSpec>&);
    void		setUserRefs(int id,int attrib,BufferStringSet*);
    bool		interpolationEnabled(int id) const;
			/*!<Specifies that the data is integers that should
			    be interpolated. */
    void		enableInterpolation(int id,bool yn);
			/*!<Specify that the data is integers that should
			    be interpolated. */
    bool		isAngle(int id,int attrib) const;
			/*!<Specifies that the data is angles, i.e. -PI==PI. */
    void		setAngleFlag(int id, int attrib, bool yn);
			/*!<Specify that the data is angles, i.e. -PI==PI. */
    bool		isAttribEnabled(int id,int attrib) const;
    void		enableAttrib(int id,int attrib,bool yn);
    bool		hasSingleColorFallback(int id) const;
    void		setTranslation(int visid,const Coord3& shift);
    Coord3		getTranslation(int visid) const;

			//Volume data stuff
    TrcKeyZSampling	getTrcKeyZSampling(int id,int attrib=-1) const;
    const RegularSeisDataPack* getCachedData(int id,int attrib) const;
    bool		setCubeData(int id,int attrib,
				    const RegularSeisDataPack*);
			/*!< data becomes mine */
    bool		setDataPackID(int id,int attrib,DataPack::ID);
    DataPack::ID	getDataPackID(int id,int attrib) const;
    DataPack::ID	getDisplayedDataPackID(int id,int attrib) const;
    DataPackMgr::MgrID	getDataPackMgrID(int id) const;
    int			currentVersion(int id,int attrib) const;

			//Trace data
    void		getDataTraceBids(int id,TypeSet<BinID>&) const;
    Interval<float>	getDataTraceRange(int id) const;

			// See visSurvey::SurfaceDisplay for details
    void		getRandomPos(int visid,DataPointSet&) const;
    void		getRandomPosCache(int visid,int attrib,
					  DataPointSet& ) const;
    void		setRandomPosData(int visid, int attrib,
					 const DataPointSet*);

    bool		hasMaterial(int id) const;
    void		setMaterial(int id);
    bool		hasColor(int id) const;
    void		setColor(int id,const OD::Color&);

    bool		blockMouseSelection(bool yn);
			/*!<\returns Previous status. */

    bool		disabMenus(bool yn);
			/*!<\returns The previous status. */
    void		createToolBars();
    bool		disabToolBars(bool yn);
			/*!<\returns The previous status. */

    bool		showMenu(int id,int menutype=0,const TypeSet<int>* =0,
				 const Coord3& = Coord3::udf());
			/*!<
			  \param id
			  \param menutype Please refer to \ref
				uiMenuHandler::executeMenu for a detailed
				description.
			*/

    MenuHandler*	getMenuHandler();
    MenuHandler*	getToolBarHandler();

    MultiID		getMultiID(int) const;

    int			getSelObjectId() const;
    int			getSelAttribNr() const;
    void		setSelObjectId(int visid,int attrib=-1);
    void		setCurInterObjID(int visid);
    int			getCurInterObjID() const;
    int			getSceneID(int visid) const;
    const ZDomain::Info* zDomainInfo(int sceneid) const;
			/*!< Returns Z domain info of scene */

			//Events and their functions
    void		unlockEvent();
			/*!< This function _must_ be called after
			     the object has sent an event to unlock
			     the object. */
    int			getEventObjId() const;
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
    void		calculateAllAttribs(int);
    bool		calculateAttrib(int id,int attrib,bool newsel,
					bool ignorelocked=false);
    bool		calcManipulatedAttribs(int id);

    void		movePlaneAndCalcAttribs(int,const TrcKeyZSampling&);

    bool		canHaveMultipleTextures(int) const;
    int			nrTextures(int id,int attrib) const;
    void		selectTexture(int id,int attrib,int texture);
    int			selectedTexture(int id,int attrib) const;

    static int		evMouseMove();
    Coord3		getMousePos() const;
    int			zFactor() const			{ return zfactor_; }
    BufferString	getMousePosVal() const;
    BufferString	getMousePosString() const	{ return mouseposstr_; }
    void		getObjectInfo(int id,BufferString&) const;

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
    BufferString		getInteractionMsg(int id) const;
				/*!< Returns dragger position or
				     Nr positions in picksets */

    static int			evViewAll();
    static int			evToHomePos();

				// ColorTable stuff
    void			fillDispPars(int id,int attrib,
					 FlatView::DataDispPars&,bool) const;
    const ColTab::MapperSetup*	getColTabMapperSetup(int id,int attrib,
						 int version=mUdf(int)) const;
    void			setColTabMapperSetup(int id,int attrib,
						    const ColTab::MapperSetup&);
    const ColTab::Sequence*	getColTabSequence(int id,int attrib) const;
    bool			canSetColTabSequence(int id) const;
    void			setColTabSequence(int id,int attrib,
						  const ColTab::Sequence&);
    bool			canHandleColTabSeqTrans(int id,int attr) const;

    const TypeSet<float>*	getHistogram(int id,int attrib) const;

    void			displayMapperRangeEditForAttrbs(int id);
    void			displayMapperRangeEditForAttribs(int id,
								 int attribid);

    static int			evColorTableChange();
    void			displaySceneColorbar(bool);
    void			manageSceneColorbar(int);
    bool			sceneColorbarDisplayed();

    OD::Color			getSceneAnnotCol(int);

				//General stuff
    bool			deleteAllObjects();
    void			setZStretch();
    void			setDirectionalLight();
    bool			setWorkingArea();
    bool			setWorkingArea(const TrcKeyZSampling&);
    void			setOnlyAtSectionsDisplay(int id,bool);
    bool			displayedOnlyAtSections(int id) const;
    static int			evViewModeChange();
    void			setViewMode(bool yn,bool notify=true);
    void			setSoloMode(bool,TypeSet< TypeSet<int> >,int);
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
    void			setZAxisTransform(int sceneid,ZAxisTransform*,
						  TaskRunner*);
    const ZAxisTransform*	getZAxisTransform(int sceneid) const;
    visBase::EventCatcher*	getEventCatcher(int sceneid);

    const Selector<Coord3>*	getCoordSelector(int scene) const;
    void			turnOn(int,bool,bool doclean=false);
    bool			isOn(int) const;
    void			updateDisplay(bool,int,int refid=-1);
    void			setTopBotImg(int sceneid);

    bool			canDuplicate(int) const;
    int				duplicateObject(int id,int sceneid);
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

    bool			writeSceneToFile(int id,
						const uiString& dlgtitle) const;

    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    bool			canBDispOn2DViewer(int id) const;
    bool			isVerticalDisp(int id) const;

    void			lock(int id,bool yn);
    bool			isLocked(int id) const;

    bool			sendVisEvent(int);
    void			setMoreObjectsToDoHint(int sceneid,bool yn);
    bool			getMoreObjectsToDoHint(int sceneid) const;
    Notifier<uiVisPartServer>	planeMovedEvent;

protected:

    void			createMenuCB(CallBacker*);
    void			addToToolBarCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    visSurvey::Scene*		getScene(int);
    const visSurvey::Scene*	getScene(int) const;

    bool			selectAttrib(int id, int attrib);
    void			updateManipulatorStatus(visBase::DataObject*,
							bool issel) const;

    void			setMarkerPos(const TrcKeyValue&,
					     int dontsetscene);

    bool			isManipulated(int id) const;
    void			acceptManipulation(int id);
    bool			resetManipulation(int id);

    void			setUpConnections(int id);
				/*!< Should set all cbs for the object */
    void			removeConnections(int id);

    void			updateDraggers();
    int				getTypeSetIdx(int);

    ObjectSet<visSurvey::Scene>	scenes_;

    uiMenuHandler&		menu_;
    uiTreeItemTBHandler*	toolbar_;

    uiMPEMan*			mpetools_;
    uiSlicePos3DDisp*		slicepostools_;
    uiSurvTopBotImageDlg* topbotdlg_ = nullptr;

    uiMultiMapperRangeEditWin*	multirgeditwin_;
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
    int				eventobjid_;
    int				eventattrib_;
    int				selattrib_;
    int				mapperrgeditordisplayid_;
    int				curinterpobjid_;

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

    TypeSet< TypeSet<int> >	displayids_;

    static const char*		sKeyWorkArea();
    static const char*		sKeyAppVel();

    uiVisModeMgr*		vismgr_;
    bool			blockmenus_;
    uiVisPickRetriever*		pickretriever_;
    Notifier<uiVisPartServer>	nrscenesChange;

    MouseCursorExchange*	mousecursorexchange_;

    uiDirLightDlg*		dirlightdlg_;

    void			triggerObjectMoved(int id);

public:
};


mClass(uiVis) uiVisModeMgr
{
public:
				uiVisModeMgr(uiVisPartServer*);
				~uiVisModeMgr() {}

	bool			allowTurnOn(int,bool);

protected:

	uiVisPartServer&	visserv;
};


#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "cubesampling.h"
#include "datapack.h"
#include "menuhandler.h"
#include "ranges.h"
#include "thread.h"
#include "uiapplserv.h"

class BinIDValueSet;
class BufferStringSet;
class Color;
class DataPointSet;
class MouseCursorExchange;
class MultiID;
class PickSet;
class SeisTrcBuf;
class SurfaceInfo;
class TaskRunner;
class ZAxisTransform;
class uiMenuHandler;
class uiMPEMan;
class uiMultiMapperRangeEditWin;
class uiPopupMenu;
class uiSlicePos3DDisp;
class uiToolBar;
class uiTreeItemTBHandler;
class uiVisModeMgr;
class uiVisPickRetriever;
class uiDirLightDlg;
template <class T> class Selector;

namespace Attrib    { class SelSpec; class DataCubes; }
namespace FlatView  { class DataDispPars; }
namespace Threads   { class Mutex; }
namespace visBase   { class DataObject; class EventCatcher; }
namespace visSurvey { class Scene; }
namespace ColTab    { class Sequence; class MapperSetup; }
namespace ZDomain   { class Info; }


/*! \brief The Visualisation Part Server */

mClass(uiVis) uiVisPartServer : public uiApplPartServer
{
    friend class 	uiMenuHandler;
    friend class        uiVisModeMgr;

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
    void		setObjectName(int,const char*);
    const char*		getObjectName(int) const;

    Notifier<uiVisPartServer>	objectaddedremoved;

    void		removeSelection();

    int			addScene(visSurvey::Scene* =0);
    			/*!<Adds a scene. The argument is only used internally.
			    Don't use the argument when calling from outside.
			*/
    void		removeScene(int);
    NotifierAccess&	nrScenesChange() { return nrsceneschange_; }
    bool		clickablesInScene(const char* trackertype, 
					  int sceneid) const;
    const ObjectSet<visSurvey::Scene>& getAllScenes() const { return scenes_; }

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
    int			addAttrib(int id);
    void		removeAttrib(int id,int attrib);
    int			getNrAttribs(int id) const;
    bool		swapAttribs(int id,int attrib0,int attrib1);
    void		showAttribTransparencyDlg(int id,int attrib);
    unsigned char	getAttribTransparency(int id,int attrib) const;
    void		setAttribTransparency(int id,int attrib, unsigned char);
    const Attrib::SelSpec* getSelSpec(int id,int attrib) const;
    void		setSelSpec(int id,int attrib,const Attrib::SelSpec&);
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
    void		setTranslation(int visid,const Coord3& shift);
    Coord3		getTranslation(int visid) const;
    
			//Volume data stuff
    CubeSampling	getCubeSampling(int id,int attrib=-1) const;
    const Attrib::DataCubes* getCachedData(int id,int attrib) const;
    bool		setCubeData(int id,int attrib,const Attrib::DataCubes*);
    			/*!< data becomes mine */
    bool		setDataPackID(int id,int attrib,DataPack::ID);
    DataPack::ID	getDataPackID(int id,int attrib) const;
    DataPackMgr::ID	getDataPackMgrID(int id) const;
    void		createAndDispDataPack(int id,int attrib,
	    				      const DataPointSet* );

    			//Trace data
    void		getDataTraceBids(int id,TypeSet<BinID>&) const;
    Interval<float>	getDataTraceRange(int id) const;
    void		setTraceData(int id,int attrib,SeisTrcBuf&);

    			// See visSurvey::SurfaceDisplay for details
    void		getRandomPos(int visid,DataPointSet&) const;
    void		getRandomPosCache(int visid,int attrib,
	    				  DataPointSet& ) const;
    void		setRandomPosData(int visid, int attrib,
					 const DataPointSet*);

    bool		blockMouseSelection(bool yn);
			/*!<\returns Previous status. */

    bool		disabMenus(bool yn);
			/*!<\returns The previous status. */
    void		createToolBars();
    bool		disabToolBars(bool yn);
			/*!<\returns The previous status. */

    bool		showMenu(int id,int menutype=0,const TypeSet<int>* =0,
	    			 const Coord3& = Coord3::udf());
    			/*!<\param menutype Please refer to \ref
				uiMenuHandler::executeMenu for a detailed
				description.
			*/

    MenuHandler*	getMenuHandler();
    MenuHandler*	getToolBarHandler();

    MultiID		getMultiID(int) const;
	
    int			getSelObjectId() const;
    int			getSelAttribNr() const;
    void		setSelObjectId(int visid,int attrib=-1);
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
    bool		calculateAttrib(int id,int attrib,bool newsel,
	    				bool ignorelocked=false);
    bool		canHaveMultipleTextures(int) const;
    int			nrTextures(int id,int attrib) const;
    void		selectTexture(int id,int attrib,int texture);
    int			selectedTexture(int id,int attrib) const;

    static int		evMouseMove();
    Coord3		getMousePos(bool xyt) const;
			/*!< If !xyt mouse pos will be in inl, crl, t */
    int			zFactor() const			{ return zfactor_; }
    BufferString	getMousePosVal() const;
    BufferString	getMousePosString() const	{ return mouseposstr_; }
    void		getObjectInfo(int id,BufferString&) const;


    static int		evSelectAttrib();

    static int		evInteraction();
    			/*<! Get the id with getEventObjId() */
    BufferString	getInteractionMsg(int id) const;
    			/*!< Returns dragger position or
			     Nr positions in picksets */

    static int		evViewAll();
    static int		evToHomePos();

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

    static int			evColorTableChange();
    void			displaySceneColorbar(bool);
    void			manageSceneColorbar(int);
    bool			sceneColorbarDisplayed();

    const Color&		getSceneAnnotCol(int);

				//General stuff
    bool			deleteAllObjects();
    void			setZStretch();
    void			setDirectionalLight();
    bool			setWorkingArea();
    static int			evViewModeChange();
    void			setViewMode(bool yn,bool notify=true);
    void			setSoloMode(bool,TypeSet< TypeSet<int> >,int);
    bool                        isSoloMode() const;
    bool			isViewMode() const;
    typedef enum		{ View, Interactive, Pick } WorkMode;
    void			setWorkMode(WorkMode,bool notify=true);
    WorkMode			getWorkMode() const;
    enum			SelectionMode { Polygon, Rectangle };
    void			setSelectionMode(SelectionMode);
    SelectionMode		getSelectionMode() const;
    void			turnSelectionModeOn(bool);
    bool			isSelectionModeOn() const;
    Notifier<uiVisPartServer>	selectionmodechange;
    void			setZAxisTransform(int sceneid,ZAxisTransform*,
						  TaskRunner*);
    ZAxisTransform*		getZAxisTransform(int sceneid);
    visBase::EventCatcher*	getEventCatcher(int sceneid);

    const Selector<Coord3>*	getCoordSelector(int scene) const;
    void			turnOn(int,bool,bool doclean=false);
    bool			isOn(int) const;
    void			updateDisplay(bool,int,int refid=-1);
    void			setTopBotImg(int sceneid);

    bool			canDuplicate(int) const;
    int				duplicateObject(int id,int sceneid);
    				/*!< \returns id of new object */

    				// Headon intensity event-related
    float			sendGetHeadOnIntensityEvent(int sceneid);
    void			sendSetHeadOnIntensityEvent(int sceneid, 
	    						    float val);
    float			getHeadOnIntensity() const;
    void			setHeadOnIntensity(float val);
    static int			evGetHeadOnIntensity();
    static int			evSetHeadOnIntensity();

    				// Tracking stuff
    void			turnSeedPickingOn(bool yn);
    static int			evPickingStatusChange();
    bool			sendPickingStatusChangeEvent(); 
    static int			evDisableSelTracker();
    bool			sendDisableSelTrackerEvent();
    void			trackInVolume(); 

    void			reportTrackingSetupActive(bool yn);
    bool 			isTrackingSetupActive() const;

    bool			isPicking() const;
    				/*!<\returns true if the selected object
				     is handling left-mouse picks on other
				     objects, so the picks won't be handled by
				     the selman. */
    void			turnQCPlaneOff();
    void			getPickingMessage(BufferString&) const;

    static int			evLoadPostponedData();
    void 			loadPostponedData() const;

    static int			evPostponedLoadingData();
    void			postponedLoadingData() const;
    
    static int			evToggleBlockDataLoad();
    void 			toggleBlockDataLoad() const;

    static int			evShowSetupDlg();
    bool			sendShowSetupDlgEvent();
    
    void			showMPEToolbar(bool yn=true);
    void			updateMPEToolbar();
    void			updateSeedConnectMode();
    void			introduceMPEDisplay();
    uiToolBar*			getTrackTB() const;
    void			initMPEStuff();
    void			fireLoadAttribDataInMPEServ();
    static int		        evLoadAttribDataInMPEServ();
    void			updateOldActiVolInuiMPEMan();
    void			restoreActiveVolInuiMPEMan();
    static int			evFromMPEManStoreEMObject();
    void			fireFromMPEManStoreEMObject();

    uiSlicePos3DDisp*		getUiSlicePos() const
    				{ return slicepostools_; }

    uiToolBar*			getItemTB() const	{ return itemtools_; }

    bool			writeSceneToFile(int id,
	    					 const char* dlgtitle) const;
    
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

    bool			canBDispOn2DViewer(int id) const;
    bool			isVerticalDisp(int id) const;

    void			lock(int id,bool yn);
    bool			isLocked(int id) const;
    
protected:

    void			createMenuCB(CallBacker*);
    void			addToToolBarCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    visSurvey::Scene*		getScene(int);
    const visSurvey::Scene*	getScene(int) const;

    bool			selectAttrib(int id, int attrib);

    void			setMarkerPos(const Coord3&,int dontsetscene);

    bool			isManipulated(int id) const;
    void			acceptManipulation(int id);
    bool			resetManipulation(int id);

    bool			hasMaterial(int id) const;
    bool			setMaterial(int id);

    bool			hasColor(int id) const;

    void			setUpConnections(int id);
    				/*!< Should set all cbs for the object */
    void			removeConnections(int id);

    void			updateDraggers();
    int				getTypeSetIdx(int);

    ObjectSet<visSurvey::Scene>	scenes_;

    uiMenuHandler&		menu_;
    uiTreeItemTBHandler&	toolbar_;

    uiMPEMan*			mpetools_;
    uiSlicePos3DDisp*		slicepostools_;
    uiToolBar*			itemtools_;

    uiMultiMapperRangeEditWin*	multirgeditwin_;
    bool			mapperrgeditinact_;

    Coord3			xytmousepos_;
    Coord3			inlcrlmousepos_;
    int				zfactor_;
    BufferString		mouseposval_;
    BufferString		mouseposstr_;

    bool			tracksetupactive_;
    bool			viewmode_;
    WorkMode			workmode_;
    bool			issolomode_;
    Threads::Mutex&		eventmutex_;
    int				eventobjid_;
    int				eventattrib_;
    int				selattrib_;
    int				mapperrgeditordisplayid_;

    int				seltype_;
    SelectionMode		selectionmode_;

    void			mouseCursorCB(CallBacker*);
    void			rightClickCB(CallBacker*);
    void			selectObjCB(CallBacker*);
    void			deselectObjCB(CallBacker*);
    void			interactionCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			vwAll(CallBacker*);
    void			toHome(CallBacker*);
    void			colTabChangeCB(CallBacker*);

    void			mapperRangeEditChanged(CallBacker*);

    MenuItem			resetmanipmnuitem_;
    MenuItem			changematerialmnuitem_;
    MenuItem			resmnuitem_;

    TypeSet< TypeSet<int> >	displayids_;

    static const char*		sKeyWorkArea();
    static const char*		sKeyAppVel();

    uiVisModeMgr*		vismgr_;
    bool			blockmenus_;
    uiVisPickRetriever*		pickretriever_;
    Notifier<uiVisPartServer>	nrsceneschange_;

    MouseCursorExchange*	mousecursorexchange_;

    uiDirLightDlg*		dirlightdlg_;
};


/*!\mainpage Visualisation User Interface

  This module provides the plain user interface classes necessary to do the
  3D visualisation in the way that the user wants.

  Main task of this server is adding and removing scene objects and 
  transfer of data to be displayed. All supported scene objects are inheriting
  visSurvey::SurveyObject.

  A lot of user interaction is done via popupmenus, and all objects share
  one uiMenuHandler that is accessed via getMenuHandler. To add items or
  manipulate the menus, please refer to the uiMenuHandler documentation.
*/


class uiVisModeMgr 
{
public:
				uiVisModeMgr(uiVisPartServer*);
				~uiVisModeMgr() {};

	bool			allowTurnOn(int,bool);

protected:

	uiVisPartServer&	visserv;	
};

#endif


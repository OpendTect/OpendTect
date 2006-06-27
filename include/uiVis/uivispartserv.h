#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.174 2006-06-27 12:44:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "menuhandler.h"
#include "ranges.h"
#include "sets.h"
#include "thread.h"
#include "uiapplserv.h"

class BinIDValueSet;
class BufferStringSet;
class ColorTable;
class MultiID;
class PickSet;
class SeisTrcBuf;
class SurfaceInfo;
class uiMPEMan;
class uiPopupMenu;
class uiToolBar;
class uiMenuHandler;
class uiVisModeMgr;
class Color;

namespace Attrib    { class SelSpec; class DataCubes; }
namespace visBase   { class DataObject; };
namespace visSurvey { class Scene; };
namespace Threads   { class Mutex; };
namespace Tracking  { class TrackManager; };



/*! \brief The Visualisation Part Server */

class uiVisPartServer : public uiApplPartServer
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
				    must then be unrefed.
			*/

    visBase::DataObject* getObject( int id ) const;
    int			highestID() const;
    void		addObject( visBase::DataObject*, int sceneid,
				   bool saveinsessions  );
    void		shareObject( int sceneid, int id );
    void		findObject( const std::type_info&, TypeSet<int>& );
    void		findObject( const MultiID&, TypeSet<int>& );
    void		removeObject( visBase::DataObject*,int sceneid);
    void		removeObject(int id,int sceneid);
    void		setObjectName(int,const char*);
    const char*		getObjectName(int) const;

    int			addScene(visSurvey::Scene* =0);
    			/*!<Adds a scene. The argument is only used internally.
			    Don't use the argument when calling from outside.
			*/
    void		removeScene(int);

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

    AttribFormat	getAttributeFormat(int id) const;
    bool		canHaveMultipleAttribs(int id) const;
    int			addAttrib(int id);
    void		removeAttrib(int id,int attrib);
    int			getNrAttribs(int id) const;
    bool		swapAttribs(int id,int attrib0,int attrib1);
    void		showAttribTransparencyDlg(int id,int attrib);
    unsigned char	getAttribTransparency(int id,int attrib) const;
    void		setAttribTransparency(int id,int attrib, unsigned char);
    const Attrib::SelSpec* getSelSpec(int id,int attrib) const;
    void		setSelSpec(int id,int attrib,const Attrib::SelSpec&);
    bool		isClassification(int id,int attrib) const;
    void		setClassification(int id, int attrib, bool yn);
    bool		isAttribEnabled(int id,int attrib) const;
    void		enableAttrib(int id,int attrib,bool yn);
    
			//Volume data stuff
    CubeSampling	getCubeSampling(int id,int attrib=-1) const;
    const Attrib::DataCubes* getCachedData(int id,int attrib) const;
    bool		setCubeData(int id,int attrib,const Attrib::DataCubes*);
    			/*!< data becomes mine */

    			//Trace data
    void		getDataTraceBids(int id,TypeSet<BinID>&) const;
    Interval<float>	getDataTraceRange(int id) const;
    void		setTraceData(int id,int attrib,SeisTrcBuf&);

    			// See visSurvey::SurfaceDisplay for details
    void		getRandomPos(int visid,ObjectSet<BinIDValueSet>&) const;
    void		getRandomPosCache(int visid,int attrib,
	    				ObjectSet<const BinIDValueSet>& ) const;
    void		setRandomPosData(int visid, int attrib,
					 const ObjectSet<BinIDValueSet>*);

    bool		blockMouseSelection(bool yn);
			/*!<\returns Previous status. */

    bool		disabMenus(bool yn);
			/*!<\returns The previous status. */
    bool		disabToolbars(bool yn);
			/*!<\returns The previous status. */

    bool		showMenu(int id,int menutype=0,const TypeSet<int>* =0,
	    			 const Coord3& = Coord3::udf());
    			/*!<\param menutype Please refer to \ref
				uiMenuHandler::executeMenu for a detailed
				description.
			*/

    MenuHandler*	getMenuHandler();

    MultiID		getMultiID(int) const;
	
    int			getSelObjectId() const;
    int			getSelAttribNr() const;
    void		setSelObjectId(int visid,int attrib=-1);
    int			getSceneID(int visid) const;
    const char*		getDepthDomainKey(int sceneid) const;
    			/*!< Returns depthdomain key of scene */

    			//Events and their functions
    void		unlockEvent();
    			/*!< This function _must_ be called after
			     the object has sent an event to unlock
			     the object. */
    int			getEventObjId() const;
    			/*<\returns the id that triggered the event */
    int			getEventAttrib() const;
    			/*<\returns the attrib that triggered the event */

    static const int	evUpdateTree;
    void		triggerTreeUpdate();

    static const int	evSelection;
    			/*<! Get the id with getEventObjId() */

    static const int	evDeSelection;
    			/*<! Get the id with getEventObjId() */

    static const int	evGetNewData;
    			/*!< Get the id with getEventObjId() */
    			/*!< Get the attrib with getEventAttrib() */
    			/*!< Get selSpec with getSelSpec */

    void		calculateAllAttribs();
    bool		calculateAttrib( int id, int attrib, bool newsel);

    bool		canHaveMultipleTextures(int) const;
    int			nrTextures( int id, int attrib ) const;
    void		selectTexture(int id, int attrib, int texture);
    int			selectedTexture( int id, int attrib ) const;

    static const int	evMouseMove;
    Coord3		getMousePos(bool xyt) const;
			/*!< If !xyt mouse pos will be in inl, crl, t */
    float		zFactor() const			{ return zfactor_; }
    BufferString	getMousePosVal() const;
    BufferString	getMousePosString() const	{ return mouseposstr_; }


    static const int	evSelectAttrib;

    static const int	evInteraction;
    			/*<! Get the id with getEventObjId() */
    BufferString	getInteractionMsg(int id) const;
    			/*!< Returns dragger position or
			     Nr positions in picksets */

    static const int	evViewAll;
    static const int	evToHomePos;

    			// ColorTable stuff
    int				getColTabId(int id, int attrib ) const;
    void			setClipRate(int id, int attrib, float);
    const TypeSet<float>*	getHistogram(int id, int attrib ) const;

				//General stuff
    bool			deleteAllObjects();
    void			setZScale();
    bool			setWorkingArea();
    void			setViewMode(bool yn);
    void			setSoloMode(bool, TypeSet< TypeSet<int> >, int);
    bool                        isSoloMode() const;
    bool			isViewMode() const;
    void			turnOn(int,bool,bool doclean=false);
    bool			isOn(int) const;
    void			updateDisplay(bool, int, int refid =-1);

    bool			canDuplicate(int) const;
    int				duplicateObject(int id,int sceneid);
    				/*!< \returns id of new object */

    bool			isLocked(int) const;

    				// Tracking stuff
    void			turnSeedPickingOn(bool yn);
    static const int		evPickingStatusChange;
    bool			isPicking() const;
    				/*!<\returns true if the selected object
				     is handling left-mouse picks on other
				     objects, so the picks won't be handled by
				     the selman. */

    static const int		evLoadPostponedData;
    void 			loadPostponedData() const;

    static const int		evShowSetupDlg;
    bool			sendShowSetupDlgEvent();
    
    void			showMPEToolbar();
    void			updateMPEToolbar();
    uiToolBar*			getTrackTB() const;
    void			initMPEStuff();

    void			makeSectionDisplayRefresh() const;
    bool			dumpOI(int id) const;
    
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:

    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    visSurvey::Scene*		getScene(int);
    const visSurvey::Scene*	getScene(int) const;

    bool			selectAttrib(int id, int attrib);

    bool			isManipulated(int id) const;
    void			acceptManipulation(int id);
    bool			resetManipulation(int id);

    bool			hasMaterial(int id) const;
    bool			setMaterial(int id);

    bool			hasColor(int id) const;

    void			setUpConnections(int id);
    				/*!< Should set all cbs for the object */
    void			removeConnections(int id);

    void			toggleDraggers();
    int				getTypeSetIdx(int);

    ObjectSet<visSurvey::Scene>	scenes_;

    uiMenuHandler&		menu_;

    uiMPEMan*			mpetools_;

    Coord3			xytmousepos_;
    Coord3			inlcrlmousepos_;
    float			zfactor_;
    BufferString		mouseposval_;
    BufferString		mouseposstr_;

    bool			viewmode_;
    bool			issolomode_;
    Threads::Mutex&		eventmutex_;
    int				eventobjid_;
    int				eventattrib_;
    int				selattrib_;

    void			rightClickCB(CallBacker*);
    void			selectObjCB(CallBacker*);
    void			deselectObjCB(CallBacker*);
    void			interactionCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			vwAll(CallBacker*);
    void			toHome(CallBacker*);
    void			colTabChangeCB(CallBacker*);

    MenuItem			resetmanipmnuitem_;
    MenuItem			changecolormnuitem_;
    MenuItem			changematerialmnuitem_;
    MenuItem			resmnuitem_;

    TypeSet< TypeSet<int> >	displayids_;

    static const char*		sKeyWorkArea();
    static const char*		sKeyAppVel();

    uiVisModeMgr*		vismgr_;
    bool			blockmenus_;
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

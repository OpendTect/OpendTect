#ifndef uivispartserv_h
#define uivispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.116 2004-04-28 21:30:58 bert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "sets.h"
#include "thread.h"
#include "position.h"

#include "cubesampling.h"
#include "attribsel.h"

class SeisTrc;
class PickSet;
class ColorTable;
class uiPopupMenu;
class SurfaceInfo;
class AttribSliceSet;
class BufferStringSet;

namespace visBase
{
    class DataObject;
};

namespace visSurvey
{
class Scene;
};

namespace Threads { class Mutex; };
namespace Geometry { class GridSurface; };

class uiVisMenuFactory;


/*! \brief The Visualisation Part Server */

class uiVisPartServer : public uiApplPartServer
{
    friend class 	uiVisMenu;

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
    void		addObject( visBase::DataObject*, int sceneid,
				   bool saveinsessions  );
    void		shareObject( int sceneid, int id );
    void		findObject( const std::type_info&, TypeSet<int>& );
    void		removeObject( visBase::DataObject*,int sceneid);
    void		removeObject(int id,int sceneid);
    void		setObjectName(int,const char*);
    const char*		getObjectName(int) const;

    int			addScene();
    void		removeScene(int);

    void		getChildIds(int id,TypeSet<int>&) const;
			/*!< Gets a scenes' children or a volumes' parts
			     If id==-1, it will give the ids of the
			     scenes */

    int			getAttributeFormat(int id) const;
   			/*!\retval 0 volume
  			   \retval 1 traces
		           \retval 2 random positions */
    const AttribSelSpec* getSelSpec(int id) const;
    const ColorAttribSel* getColorSelSpec(int id) const;
    void		setSelSpec(int id, const AttribSelSpec&);
    void		setColorSelSpec(int id, const ColorAttribSel& );
    
			//Volume data stuff
    CubeSampling	getCubeSampling(int id) const;
    const AttribSliceSet* getCachedData(int id,bool color) const;
    bool		setCubeData(int id, bool color,AttribSliceSet*);
    			/*!< data becomes mine */

    			//Trace data
    void		getDataTraceBids(int id, TypeSet<BinID>&) const;
    Interval<float>	getDataTraceRange(int id) const;
    void		setTraceData(int id, bool color, ObjectSet<SeisTrc>* );
			//!< Traces become mine

    			//Random position data
    void		getRandomPosDataPos(int id,
				    ObjectSet<TypeSet<BinIDZValues> >&,
				    bool inclvals=false) const;
    			/*!< Content of objectset becomes callers */
    void		setRandomPosData(int id, bool color, const ObjectSet<
	    			const TypeSet<const BinIDZValues> >* );
    			/*!< The data should have exactly the same
			     structure as the positions given in
			     getRandomPosDataPos */

    bool		showMenu( int id );
    uiVisMenuFactory*	getMenuFactory(int id, bool create=true);

    int				addInlCrlTsl(int scene,int type);
    int				addRandomLine(int scene);
    int				addVolView(int scene);
    int				addSurface(int scene,const MultiID&);
    void			getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void			shiftHorizon(int id,float);
    int				addWell(int scene,const MultiID&);
    void			displayLog(int id,int,int);
    void			refreshMarkers();

    int				addPickSet(int scene,const PickSet&);
    void			getAllPickSets(BufferStringSet&);
    void			setPickSetData(int,const PickSet&);
    void			getPickSetData(int,PickSet&) const;

    int				addStickSet(int scene,const MultiID&);
    MultiID			getMultiID(int) const;
    int				getObjectId(int scene,const MultiID&) const;
	
    int				getSelObjectId() const;
    void			setSelObjectId(int);

    void			makeSubMenu(uiPopupMenu&,int scene,int id);
    				/*!< Gives menu ids from 1024 and above */
    bool			handleSubMenuSel(int mnu,int scene,int id);

    				//Events and their functions
    void			unlockEvent();
    				/*!< This function _must_ be called after
				     the object has sent an event to unlock
				     the object. */
    int				getEventObjId() const;
    				/*<\returns the id that triggered the event */
    int				getEventAttrNr() const;
				/*<\returns the attr that triggered the event*/

    static const int		evUpdateTree;
    BufferString		getTreeInfo(int id) const;
    BufferString		getDisplayName(int) const;

    bool			isInlCrlTsl(int,int dim) const;
    				/*!< dim==-1 : any slice
				     dim==0 : inline
				     dim==1 : crossline
				     dim==2 : tslc */
    bool			isVolView(int) const;
    bool			isRandomLine(int) const;
    bool			isHorizon(int) const;
    bool			isFault(int) const;
    bool			isStickSet(int) const;
    bool			isWell(int) const;
    bool			isPickSet(int) const;
    bool			hasAttrib(int) const;

    static const int		evSelection;
    				/*<! Get the id with getEventObjId() */

    static const int		evDeSelection;
    				/*<! Get the id with getEventObjId() */

    static const int		evGetNewData;
    				/*<! Get the id with getEventObjId() */
    				/*!< Get selSpec with getSelSpec */
    bool			calculateAttrib(int id,bool newsel);
    bool			calculateColorAttrib(int,bool);
    void			showTexture(int,int);

    static const int		evMouseMove;
    Coord3			getMousePos(bool xyt) const;
				/*!< If !xyt mouse pos will be in inl, crl, t */
    BufferString		getMousePosVal() const;


    static const int		evSelectAttrib;

    static const int		evSelectColorAttrib;
    static const int		evGetColorData;
    void			setColorSelSpec(const ColorAttribSel&);
    void			setColorData(int,AttribSliceSet*);
    void			resetColorDataType(int);

    static const int		evInteraction;
    				/*<! Get the id with getEventObjId() */
    BufferString		getInteractionMsg(int id) const;
    				/*!< Returns dragger position or
				     Nr positions in picksets */

    static const int		evViewAll;
    static const int		evToHomePos;

    				// ColorTable stuff
    int				getColTabId(int) const;
    void			setClipRate(int,float);
    const TypeSet<float>*	getHistogram(int) const;

				//General stuff
    bool			deleteAllObjects();
    void			setZScale();
    bool			setWorkingArea();
    void			setViewMode(bool yn);
    bool			isViewMode() const;
    void			turnOn(int,bool);
    bool			isOn(int) const;

    bool			canDuplicate(int) const;
    int				duplicateObject(int id,int sceneid);
    				/*!< \returns id of new object */
    
    bool			usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:

    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);

    visSurvey::Scene*		getScene(int);
    const visSurvey::Scene*	getScene(int) const;

    bool			selectAttrib(int id);

    bool			hasColorAttrib(int) const;
    bool			selectColorAttrib(int);
    void			removeColorData(int);

    bool			isMovable(int id) const;
    bool			setPosition(int id);

    bool			isManipulated(int id) const;
    void			acceptManipulation(int id);
    bool			resetManipulation(int id);

    bool			hasMaterial(int id) const;
    bool			setMaterial(int id);

    bool			hasColor(int id) const;

    void			setUpConnections(int id);
    				/*!< Should set all cbs for the object */
    void			removeConnections(int id);

    bool			dumpOI(int id) const;
    void			toggleDraggers();
    bool			loadcreateSurface(int);

    ObjectSet<visSurvey::Scene>	scenes;

    ObjectSet<uiVisMenuFactory>	menufactories;

    ColorAttribSel		colorspec;
    AttribSelSpec		attribspec;
    				/* For temporary use when sending 
				   evSelectAttrib events */

    uiVisMenu*			vismenu;

    Coord3			xytmousepos;
    Coord3			inlcrlmousepos;
    float			mouseposval;

    bool			viewmode;
    Threads::Mutex&		eventmutex;
    int				eventobjid;

    void			selectObjCB(CallBacker*);
    void			deselectObjCB(CallBacker*);
    void			interactionCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
    void			updatePlanePos(CallBacker*);
    void			vwAll(CallBacker*);
    void			toHome(CallBacker*);

    int				selcolorattrmnusel;
    int				resetmanipmnusel;
    int				changecolormnusel;
    int				changematerialmnusel;
    int				firstresmnusel;

    static const char*		workareastr;
    static const char*		appvelstr;
};


/*!\mainpage Visualisation User Interface

  This module provides the plain user interface classes necessary to do the
  3D visualisation in the way that the user wants.

  The uiVisPartServer is a rather big class, that could use a bit of redesign.
  Main task of this server is adding and removing scene objects and 
  transfer of data to be displayed. All supported scene objects are defined
  in the visSurvey module.

  A lot of user interaction is done via popupmenus. Each object has its own
  visualisation options. These options and corresponding actions are managed 
  by the uiVisMenu class.

  */

#endif

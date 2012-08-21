#ifndef uiodscenemgr_h
#define uiodscenemgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.h,v 1.90 2012-08-21 10:40:57 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodapplmgr.h"

#include "datapack.h"
#include "emposid.h"
#include "uivispartserv.h"

class BufferStringSet;
class MultiID;
class Timer;
class uiDockWin;
class uiFlatViewWin;
class uiLabel;
class uiListView;
class uiMdiArea;
class uiMdiAreaWindow;
class uiODTreeTop;
class uiSliderExtra;
class ui3DViewer;
class uiThumbWheel;
class uiTreeFactorySet;
class uiTreeItem;
class uiWindowGrabber;
class ZAxisTransform;


/*!\brief Manages the scenes and the corresponding trees.

  Most of the interface is really not useful for plugin builders.

 */

mClass(uiODMain) uiODSceneMgr : public CallBacker
{
public:

    void			cleanUp(bool startnew=true);
    int				nrScenes()	{ return scenes_.size(); }
    int				addScene(bool maximized,ZAxisTransform* =0,
	    				 const char* nm=0);
    				//!<Returns scene id
    void			removeScene(CallBacker*);
    void			setSceneName(int sceneid,const char*);
    const char*			getSceneName(int sceneid) const;
    CNotifier<uiODSceneMgr,int>	sceneClosed;
    CNotifier<uiODSceneMgr,int>	treeToBeAdded;

    void			getScenePars(IOPar&);
    void			useScenePars(const IOPar&);
    void			setSceneProperties();

    void			setToViewMode(bool yn=true);
    void			setToWorkMode(uiVisPartServer::WorkMode wm);
    void			viewModeChg(CallBacker* cb=0);
    void			actMode(CallBacker* cb=0);
    void			viewMode(CallBacker* cb=0);
    Notifier<uiODSceneMgr>	viewModeChanged;

    void			pageUpDownPressed(CallBacker*);

    void			updateStatusBar();
    void			setKeyBindings();
    void			setStereoType(int);
    int				getStereoType() const;

    void			tile();
    void			tileHorizontal();
    void			tileVertical();
    void			cascade();
    void			layoutScenes();

    void			toHomePos(CallBacker*);
    void			saveHomePos(CallBacker*);
    void			viewAll(CallBacker*);
    void			align(CallBacker*);
    void			showRotAxis(CallBacker*);
    void			viewX(CallBacker*);
    void			viewY(CallBacker*);
    void			viewZ(CallBacker*);
    void			viewInl(CallBacker*);
    void			viewCrl(CallBacker*);
    void			switchCameraType(CallBacker*);
    void			mkSnapshot(CallBacker*);
    void			soloMode(CallBacker*);
    void			doDirectionalLight(CallBacker*);

    void			setZoomValue(float);
    void			zoomChanged(CallBacker*);
    void			anyWheelStart(CallBacker*);
    void			anyWheelStop(CallBacker*);
    void			hWheelMoved(CallBacker*);
    void			vWheelMoved(CallBacker*);
    void			dWheelMoved(CallBacker*);

    int				askSelectScene() const; // returns sceneid
    const ui3DViewer*		getSoViewer(int sceneid) const;
    ui3DViewer*			getSoViewer(int sceneid);
    void			getSoViewers(ObjectSet<ui3DViewer>&);
    void			getSceneNames(BufferStringSet&,int& act) const;
    void			setActiveScene(const char* scenenm);
    void			getActiveSceneName(BufferString&) const;
    int				getActiveSceneID() const;
    Notifier<uiODSceneMgr>	activeSceneChanged;

    uiODTreeTop*		getTreeItemMgr(const uiListView*) const;

    void			displayIn2DViewer(int visid,int attribid,
	    					  bool wva);
    void			remove2DViewer(int visid);

    void			updateTrees();
    void			rebuildTrees();
    void			setItemInfo(int visid);
    void			updateSelectedTreeItem();
    void			updateItemToolbar(int visid);
    int				getIDFromName(const char*) const;
    void			disabRightClick( bool yn );
    void			disabTrees( bool yn );

    int				addWellItem(const MultiID&,int sceneid=-1);
    int				addEMItem(const EM::ObjectID&,int sceneid=-1);
    int 			addRandomLineItem(int,int sceneid=-1);
    int 			add2DLineSetItem(const MultiID&,const char*,
	    						int,int);
    void			removeTreeItem(int displayid);
    uiTreeItem*			findItem(int displayid);
    void			findItems(const char*,ObjectSet<uiTreeItem>&);

    uiTreeFactorySet*		treeItemFactorySet()	{ return tifs_; }
    uiListView*			getTree(int sceneid);

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }
    void			setViewSelectMode(int);

    float			getHeadOnLightIntensity(int) const;
    void 			setHeadOnLightIntensity(int,float);

protected:

				uiODSceneMgr(uiODMain*);
				~uiODSceneMgr();
    void			initMenuMgrDepObjs();

    void			afterFinalise(CallBacker*);
    uiThumbWheel*		dollywheel;
    uiThumbWheel*		hwheel;
    uiThumbWheel*		vwheel;
    uiLabel*			dollylbl;
    uiLabel*			dummylbl;
    uiLabel*			rotlbl;

    uiODMain&			appl_;
    uiMdiArea*			mdiarea_;
    void			mdiAreaChanged(CallBacker*);

    int				vwridx_;
    float			lasthrot_, lastvrot_, lastdval_;
    uiTreeFactorySet*		tifs_;
    uiSliderExtra*		zoomslider_;
    uiWindowGrabber*		wingrabber_;

    void			wheelMoved(CallBacker*,int wh,float&);

    inline uiODApplMgr&		applMgr()     { return appl_.applMgr(); }
    inline uiODMenuMgr&		menuMgr()     { return appl_.menuMgr(); }
    inline uiVisPartServer&	visServ()     { return *applMgr().visServer(); }

    mClass(uiODMain) Scene
    {
    public:
				Scene(uiMdiArea*);
				~Scene();
       
	uiDockWin*		dw_;
	uiListView*		lv_;
	uiMdiAreaWindow* 	mdiwin_;
	ui3DViewer*		sovwr_;
	uiODTreeTop*		itemmanager_;
    };

    ObjectSet<Scene>		scenes_;
    Scene&			mkNewScene();
    void			initTree(Scene&,int);
    Scene*			getScene(int sceneid);
    const Scene*		getScene(int sceneid) const;
    Timer*			scenetimer_;
    void			sceneTimerCB(CallBacker*);

    friend class		uiODMain;
};

#endif


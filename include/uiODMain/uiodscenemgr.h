#ifndef uiodscenemgr_h
#define uiodscenemgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.h,v 1.2 2003-12-24 15:15:42 bert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class uiGroup;
class uiODMain;
class uiDockWin;
class uiSoViewer;
class uiListView;
class uiWorkSpace;
class uiODTreeTop;
class uiTreeFactorySet;


/*!\brief Position control elements at application top level */

class uiODSceneMgr
{
public:

			uiODSceneMgr(uiODMain*);
			~uiODSceneMgr();

    void		cleanUp(bool startnew=true);
    void		addScene();
    void		removeScene(CallBacker*);

    void		getScenePars(IOPar&);
    void		mkScenesFrom(ODSession*);

    void		viewModeChg();
    void		setToViewMode();
    void		actMode();
    void		viewMode();

    void		setMousePos();
    void		setKeyBindings();
    void		setStereoViewing(bool& stereo,bool& quad);

    void		tile();
    void		cascade();
    void		layoutScenes();

    void		toHomePos(CallBacker*);
    void		saveHomePos(CallBacker*);
    void		viewAll(CallBacker*);
    void		align(CallBacker*);
    void		showRotAxis(CallBacker*);

    void		setZoomValue(float);
    void		zoomChanged(CallBacker*);
    void		anyWheelStart(CallBacker*);
    void		anyWheelStop(CallBacker*);
    void		hWheelMoved(CallBacker*);
    void		vWheelMoved(CallBacker*);
    void		dWheelMoved(CallBacker*);

    void		getSoViewers(ObjectSet<uiSoViewer>&);

    void		initTree(uiSceneGroup*,int);
    void		updateTrees();
    void		rebuildTrees();
    void		setItemInfo(int);
    void		updateSelectedTreeItem(int);
    void		getIDFromName(const char*);
    void		disabRightClick(bool);

    void		addPickSetItem(const PickSet*,int);
    void		addHorizonItem(const MultiID&,int);

protected:

    class Scene
    {
    public:
			Scene(uiWorkSpace*);
			~Scene();
       
	uiListView*	lv;
	uiDockWin*	treewin;
	uiSoViewer*	sovwr;
	uiODTreeTop*	itemmanager;

	uiGroup*	grp();
    };

    uiODMain&		appl;
    uiWorkSpace*	wsp;
    ObjectSet<Scene>	scenes;
    int			vwridx;
    int			lasthrot, lastvrot, lastdval;
    uiTreeFactorySet*	tifs;

    void		wheelMoved(CallBacker*,int wh,float&);

    inline uiODApplMgr& applMgr()	{ return appl.applMgr(); }
    inline uiODMenuMgr&	menuMgr()	{ return appl.menuMgr(); }

};



#endif

#ifndef uiodscenemgr_h
#define uiodscenemgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.h,v 1.13 2004-09-17 15:20:56 nanne Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"
class MultiID;
class PickSet;
class uiGroup;
class uiDockWin;
class uiSoViewer;
class uiListView;
class uiWorkSpace;
class uiODTreeTop;
class uiSliderExtra;
class uiVisPartServer;
class uiTreeCreaterSet;


/*!\brief Manages the scenes and the corresponding trees.

  Most of the interface is really not useful for plugin builders.

 */

class uiODSceneMgr : public CallBacker
{
public:

    void		cleanUp(bool startnew=true);
    void		addScene();
    void		removeScene(CallBacker*);

    void		getScenePars(IOPar&);
    void		useScenePars(const IOPar&);
    void		storePositions();

    void		setToViewMode(bool yn=true);
    void		viewModeChg(CallBacker* cb=0);
    void		actMode(CallBacker* cb=0);
    void		viewMode(CallBacker* cb=0);

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

    void		updateTrees();
    void		rebuildTrees();
    void		setItemInfo(int);
    void		updateSelectedTreeItem();
    int			getIDFromName(const char*) const;
    void		disabRightClick(bool);
    void		disabTree(int,bool);

    int			addPickSetItem(const PickSet*,int);
    int			addSurfaceItem(const MultiID&,int,bool);
    void		removeTreeItem(int);

    uiTreeCreaterSet*   treeItemCreaterSet() 		{ return tics; }

protected:

			uiODSceneMgr(uiODMain*);
			~uiODSceneMgr();
    void		initMenuMgrDepObjs();

    class Scene
    {
    public:
			Scene(uiWorkSpace*);
			~Scene();
       
	uiListView*	lv;
	uiSoViewer*	sovwr;
	uiODTreeTop*	itemmanager;

	uiGroup*	vwrGroup();
	uiDockWin*	treeWin();
    };

    uiODMain&		appl;
    uiWorkSpace*	wsp;
    ObjectSet<uiODSceneMgr::Scene> scenes;
    int			vwridx;
    float		lasthrot, lastvrot, lastdval;
    uiTreeCreaterSet*	tics;
    uiSliderExtra*	zoomslider;

    void		wheelMoved(CallBacker*,int wh,float&);

    inline uiODApplMgr& applMgr()	{ return appl.applMgr(); }
    inline uiODMenuMgr&	menuMgr()	{ return appl.menuMgr(); }
    inline uiVisPartServer& visServ()	{ return *applMgr().visServer(); }

    Scene&		mkNewScene();
    void		initTree(Scene&,int);

    friend class	uiODMain;

};



#endif

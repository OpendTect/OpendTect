#ifndef uiodscenemgr_h
#define uiodscenemgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.h,v 1.64 2009-03-24 04:41:08 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"

#include "datapack.h"
#include "emposid.h"

class BufferStringSet;
class MultiID;
class uiDockWin;
class uiFlatViewWin;
class uiGroup;
class uiLabel;
class uiListView;
class uiODTreeTop;
class uiSliderExtra;
class uiSoViewer;
class uiThumbWheel;
class uiTreeFactorySet;
class uiTreeItem;
class uiVisPartServer;
class uiWindowGrabber;
class uiWorkSpace;
class uiWorkSpaceGroup;
class ZAxisTransform;


/*!\brief Manages the scenes and the corresponding trees.

  Most of the interface is really not useful for plugin builders.

 */

mClass uiODSceneMgr : public CallBacker
{
public:

    void			cleanUp(bool startnew=true);
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
    void			storePositions();

    void			setToViewMode(bool yn=true);
    void			viewModeChg(CallBacker* cb=0);
    void			actMode(CallBacker* cb=0);
    void			viewMode(CallBacker* cb=0);

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
    void			selectionMode(CallBacker*);
    void			soloMode(CallBacker*);

    void			setZoomValue(float);
    void			zoomChanged(CallBacker*);
    void			anyWheelStart(CallBacker*);
    void			anyWheelStop(CallBacker*);
    void			hWheelMoved(CallBacker*);
    void			vWheelMoved(CallBacker*);
    void			dWheelMoved(CallBacker*);

    const uiSoViewer*		getSoViewer(int sceneid) const;
    void			getSoViewers(ObjectSet<uiSoViewer>&);
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
    void			setItemInfo(int);
    void			updateSelectedTreeItem();
    int				getIDFromName(const char*) const;
    void			disabRightClick( bool yn );
    void			disabTrees( bool yn );

    int				addEMItem(const EM::ObjectID&,int);
    int 			addRandomLineItem(int,int);
    void			removeTreeItem(int displayid);
    uiTreeItem*			findItem(int displayid);
    void			findItems(const char*,ObjectSet<uiTreeItem>&);

    uiTreeFactorySet*		treeItemFactorySet()	{ return tifs_; }
    uiDockWin*			viewer2DWin()		{ return 0; }

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }
    void			setViewSelectMode(int);

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

    mClass Scene
    {
    public:
			Scene(uiWorkSpace*);
			~Scene();
       
	uiDockWin*	dw_;
	uiListView*	lv_;
	uiWorkSpaceGroup* wsgrp_;
	uiSoViewer*	sovwr_;
	uiODTreeTop*	itemmanager_;
    };

    mClass Viewer2D
    {
    public:
			Viewer2D(uiODMain&,int visid);
			~Viewer2D();

	void		setUpView(DataPack::ID,bool wva,bool isvert);

	uiFlatViewWin*	viewwin_;
	uiODMain&	appl_;

	int		visid_;
	BufferString	basetxt_;
	
    protected:

	void		createViewWin(bool isvert);
    };

    uiODMain&		appl_;
    uiWorkSpace*	wsp_;
    ObjectSet<Scene>	scenes_;
    ObjectSet<Viewer2D>	viewers2d_;
    Viewer2D&		addViewer2D(int visid);
    void		wspChanged(CallBacker*);

    int				vwridx_;
    float			lasthrot_, lastvrot_, lastdval_;
    uiTreeFactorySet*		tifs_;
    uiSliderExtra*		zoomslider_;
    uiWindowGrabber*		wingrabber_;

    void			wheelMoved(CallBacker*,int wh,float&);

    inline uiODApplMgr&		applMgr()     { return appl_.applMgr(); }
    inline uiODMenuMgr&		menuMgr()     { return appl_.menuMgr(); }
    inline uiVisPartServer&	visServ()     { return *applMgr().visServer(); }

    Scene&			mkNewScene();
    void			initTree(Scene&,int);

    friend class		uiODMain;
};

#endif

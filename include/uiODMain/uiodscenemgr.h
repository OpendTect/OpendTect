#ifndef uiodscenemgr_h
#define uiodscenemgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.h,v 1.46 2007-04-02 16:36:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"

#include "datapack.h"
#include "emposid.h"


class MultiID;
class uiDockWin;
class uiGroup;
class uiListView;
class uiODTreeTop;
class uiSliderExtra;
class uiSoViewer;
class uiTreeFactorySet;
class uiTreeItem;
class uiVisPartServer;
class uiWorkSpace;
class uiFlatViewWin;
namespace Pick { class Set; }
class uiThumbWheel;


/*!\brief Manages the scenes and the corresponding trees.

  Most of the interface is really not useful for plugin builders.

 */

class uiODSceneMgr : public CallBacker
{
public:

    void			cleanUp(bool startnew=true);
    int				addScene(bool maximized);
    void			removeScene(CallBacker*);
    void			setSceneName(int sceneid,const char*);
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
    void			soloMode(CallBacker*);

    void			setZoomValue(float);
    void			zoomChanged(CallBacker*);
    void			anyWheelStart(CallBacker*);
    void			anyWheelStop(CallBacker*);
    void			hWheelMoved(CallBacker*);
    void			vWheelMoved(CallBacker*);
    void			dWheelMoved(CallBacker*);

    void			getSoViewers(ObjectSet<uiSoViewer>&);

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
    void			removeTreeItem(int displayid);
    uiTreeItem*			findItem(int displayid);

    uiTreeFactorySet*		treeItemFactorySet()	{ return tifs_; }
    uiDockWin*			viewer2DWin()		{ return 0; }

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }

protected:

				uiODSceneMgr(uiODMain*);
				~uiODSceneMgr();
    void			initMenuMgrDepObjs();

    void			afterFinalise(CallBacker*);
    uiThumbWheel*		dollywheel;
    uiThumbWheel*		hwheel;
    uiThumbWheel*		vwheel;

    class Scene
    {
    public:
			Scene(uiWorkSpace*);
			~Scene();
       
	uiListView*	lv_;
	uiSoViewer*	sovwr_;
	uiODTreeTop*	itemmanager_;

	uiGroup*	vwrGroup();
	uiDockWin*	treeWin();
    };

    class Viewer2D
    {
    public:
			Viewer2D(uiODMain&,int visid);
			~Viewer2D();

	void		setData(DataPack::ID,bool wva);

	uiFlatViewWin*	viewwin_;
	uiODMain&	appl_;

	int		visid_;
	BufferString	basetxt_;
    };

    uiODMain&		appl_;
    uiWorkSpace*	wsp_;
    ObjectSet<Scene>	scenes_;
    ObjectSet<Viewer2D>	viewers2d_;
    Viewer2D&		addViewer2D(int visid);

    int				vwridx_;
    float			lasthrot_, lastvrot_, lastdval_;
    uiTreeFactorySet*		tifs_;
    uiSliderExtra*		zoomslider_;

    void			wheelMoved(CallBacker*,int wh,float&);

    inline uiODApplMgr&		applMgr()     { return appl_.applMgr(); }
    inline uiODMenuMgr&		menuMgr()     { return appl_.menuMgr(); }
    inline uiVisPartServer&	visServ()     { return *applMgr().visServer(); }

    Scene&			mkNewScene();
    void			initTree(Scene&,int);

    friend class		uiODMain;
};

#endif

#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodapplmgr.h"
#include "uisettings.h"

#include "datapack.h"
#include "emposid.h"
#include "flatview.h"
#include "uivispartserv.h"

class BufferStringSet;
class Timer;
class ZAxisTransform;
class uiDockWin;
class uiFlatViewWin;
class uiMdiArea;
class uiMdiAreaWindow;
class uiODDisplayTreeItem;
class uiODTreeTop;
class ui3DViewer;
class uiTreeFactorySet;
class uiTreeItem;
class uiTreeView;
class uiWindowGrabber;
namespace Pick { class Set; }
namespace Geometry { class RandomLineSet; }


/*!\brief Manages the scenes and the corresponding trees.

  Most of the interface is really not useful for plugin builders.

 */

mExpClass(uiODMain) uiODSceneMgr : public CallBacker
{ mODTextTranslationClass(uiODSceneMgr)
public:

    void			cleanUp(bool startnew=true);
    int				nrScenes()	{ return scenes_.size(); }
    int				addScene(bool maximized,ZAxisTransform* =0,
				    const uiString& nm=uiString::emptyString());
				//!<Returns scene id
    void			setSceneName(int sceneid,const uiString&);
    uiString			getSceneName(int sceneid) const;
    CNotifier<uiODSceneMgr,int>	sceneClosed;
    CNotifier<uiODSceneMgr,int>	treeToBeAdded;
    CNotifier<uiODSceneMgr,int> treeAdded;
    Notifier<uiODSceneMgr>	scenesHidden;
    Notifier<uiODSceneMgr>	scenesShown;

    void			getScenePars(IOPar&);
    void			useScenePars(const IOPar&);
    void			setSceneProperties();

    void			setToViewMode(bool yn=true);
    void			setToWorkMode(uiVisPartServer::WorkMode wm);
    void			viewModeChg(CallBacker* cb=0);
    void			actMode(CallBacker* cb=0);
    void			viewMode(CallBacker* cb=0);
    bool			inViewMode() const;
    Notifier<uiODSceneMgr>	viewModeChanged;

    void			pageUpDownPressed(CallBacker*);

    void			updateStatusBar();
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
    const ui3DViewer*		get3DViewer(int sceneid) const;
    ui3DViewer*			get3DViewer(int sceneid);
    void			get3DViewers(ObjectSet<ui3DViewer>&);
    void			getSceneNames(uiStringSet&,int& act) const;
    void			setActiveScene(int idx);
    void			getActiveSceneName(BufferString&) const;
    int				getActiveSceneID() const;
    Notifier<uiODSceneMgr>	activeSceneChanged;

    uiODTreeTop*		getTreeItemMgr(const uiTreeView*) const;
    uiODTreeTop*		getTreeItemMgr(int sceneid) const;

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

    void			getLoadedPickSetIDs(TypeSet<MultiID>&,bool poly,
						    int scnid=-1) const;
    void			getLoadedEMIDs(TypeSet<EM::ObjectID>&,
					       const char* emtypestr=0,
					       int sceneid=-1) const;
				// if sceneid==-1, then all scenes
    int				addEMItem(const EM::ObjectID&,int sceneid=-1);
    int				addPickSetItem(const MultiID&,int sceneid=-1);
    int				addPickSetItem(Pick::Set&,int sceneid=-1);
    int				addRandomLineItem(int rlid,int sceneid=-1);
    int				addWellItem(const MultiID&,int sceneid=-1);
    int				add2DLineItem(Pos::GeomID,int sceneid=-1);
    int				add2DLineItem(const MultiID&,int sceneid=-1);
    int				add2DLineItem(Pos::GeomID,int sceneid,
					      bool withdata);
    int				addInlCrlItem(OD::SliceType,int nr,
					      int sceneid=-1);
    int				addZSliceItem(const TrcKeyZSampling&,
					      int sceneid=-1);
    int				addZSliceItem(const TrcKeyZSampling&,
					      const Attrib::SelSpec&,
					      int sceneid=-1);
    int				addZSliceItem(DataPack::ID,
					      const Attrib::SelSpec&,
					      const FlatView::DataDispPars::VD&,
					      int sceneid=-1);
    int				addDisplayTreeItem(uiODDisplayTreeItem*,
						   int sceneid=-1);

    void			removeTreeItem(int displayid);
    uiTreeItem*			findItem(int displayid);
    void			findItems(const char*,ObjectSet<uiTreeItem>&);
    void			findItems(const char*,ObjectSet<uiTreeItem>&,
	    				  int sceneid);

    uiTreeFactorySet*		treeItemFactorySet()	{ return tifs_; }
    uiTreeView*			getTree(int sceneid);
    void			showTree(bool yn);
    bool			treeShown() const;

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }
    void			setViewSelectMode(int);

    float			getHeadOnLightIntensity(int) const;
    void			setHeadOnLightIntensity(int,float);

    void			translateText();

    static uiString		sElements()	{ return tr("Elements"); }

protected:

				uiODSceneMgr(uiODMain*);
				~uiODSceneMgr();
    void			initMenuMgrDepObjs();

    void			afterFinalise(CallBacker*);

    uiODMain&			appl_;
    uiMdiArea*			mdiarea_;
    void			mdiAreaChanged(CallBacker*);

    int				vwridx_;
    uiTreeFactorySet*		tifs_;
    uiWindowGrabber*		wingrabber_;

    inline uiODApplMgr&		applMgr()     { return appl_.applMgr(); }
    inline uiVisPartServer&	visServ()     { return *applMgr().visServer(); }

    mExpClass(uiODMain) Scene
    {
    public:
				Scene(uiMdiArea*);
				~Scene();

	uiDockWin*		dw_;
	uiTreeView*		lv_;
	uiMdiAreaWindow*	mdiwin_;
	ui3DViewer*		vwr3d_;
	uiODTreeTop*		itemmanager_;
    };

    ObjectSet<Scene>		scenes_;
    Scene&			mkNewScene();
    void			removeScene(Scene& scene);
    void			removeSceneCB(CallBacker*);
    void			initTree(Scene&,int);
    Scene*			getScene(int sceneid);
    const Scene*		getScene(int sceneid) const;
    void			newSceneUpdated(CallBacker*);
    void			gtLoadedEMIDs(const Scene*,
					      TypeSet<EM::ObjectID>&,
					      const char* emtypestr) const;
    void			gtLoadedEMIDs(const uiTreeItem*,
					      TypeSet<EM::ObjectID>&,
					      const char* emtypestr) const;
    void			gtLoadedPickSetIDs(const Scene&,
				    TypeSet<MultiID>&, bool poly) const;
    void			gtLoadedPickSetIDs(const uiTreeItem&,
				    TypeSet<MultiID>&,bool poly) const;

    Timer*			tiletimer_;
    void			tileTimerCB(CallBacker*);

    void			font3DChanged(CallBacker*);

    friend class		uiODMain;

    void			resetStatusBar(int id=-1);
    void			showIfMinimized(CallBacker*);
};

/*! Settings Tab for mouse interaction. */

mExpClass(uiODMain) uiKeyBindingSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiKeyBindingSettingsGroup);
public:
    mDefaultFactoryInstantiation2Param( uiSettingsGroup,
				       uiKeyBindingSettingsGroup,
				       uiParent*,Settings&,
				       "Mouse interaction",
		       mToUiStringTodo(sFactoryKeyword()));

    uiKeyBindingSettingsGroup(uiParent*,Settings&);

private:
    bool		acceptOK();
    virtual HelpKey	helpKey() const;

    uiGenInput*		keybindingfld_;
    uiGenInput*		wheeldirectionfld_;
    uiGenInput*		trackpadzoomspeedfld_;

    BufferString	initialkeybinding_;
    float		initialzoomfactor_;
    bool		initialmousewheelreversal_;
};


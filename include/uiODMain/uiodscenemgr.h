#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "ui3dviewer.h"
#include "uiodapplmgr.h"
#include "uisettings.h"

#include "datapack.h"
#include "emposid.h"
#include "flatview.h"
#include "uivispartserv.h"

class BufferStringSet;
class RegularSeisDataPack;
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
    SceneID			addScene(bool maximized,
					 ZAxisTransform* =nullptr,
					 const uiString& nm=uiString::empty());
				//!<Returns scene id
    void			setSceneName(const SceneID&,const uiString&);
    uiString			getSceneName(const SceneID&) const;
    CNotifier<uiODSceneMgr,SceneID> sceneClosed;
    CNotifier<uiODSceneMgr,SceneID> treeToBeAdded;
    CNotifier<uiODSceneMgr,SceneID> treeAdded;
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
    void			resetHomePos(CallBacker*);
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

    SceneID			askSelectScene() const; // returns sceneid
    const ui3DViewer*		get3DViewer(const SceneID&) const;
    ui3DViewer*			get3DViewer(const SceneID&);
    void			get3DViewers(ObjectSet<ui3DViewer>&);
    void			getSceneNames(uiStringSet&,int& act) const;
    void			setActiveScene(int idx);
    void			setActiveScene(const SceneID&);
    void			getActiveSceneName(BufferString&) const;
    SceneID			getActiveSceneID() const;
    Notifier<uiODSceneMgr>	activeSceneChanged;

    uiODTreeTop*		getTreeItemMgr(const uiTreeView*) const;
    uiODTreeTop*		getTreeItemMgr(const SceneID&) const;

    void			displayIn2DViewer(const VisID&,int attribid,
						  bool wva);
    void			remove2DViewer(const VisID&);

    void			updateTrees();
    void			rebuildTrees();
    void			setItemInfo(const VisID&);
    void			updateSelectedTreeItem();
    void			updateItemToolbar(const VisID&);
    VisID			getIDFromName(const char*) const;
    void			disabRightClick(bool yn);
    void			disabTrees(bool yn);

    void			getLoadedPickSetIDs(TypeSet<MultiID>&,bool poly,
					const SceneID& =SceneID::udf()) const;
    void			getLoadedEMIDs(TypeSet<EM::ObjectID>&,
					const char* emtypestr=nullptr,
					const SceneID& =SceneID::udf()) const;
				// if sceneid==udf, then all scenes
    VisID			addEMItem(const EM::ObjectID&,
					  const SceneID& =SceneID::udf());
    VisID			addPickSetItem(const MultiID&,
					      const SceneID& =SceneID::udf());
    VisID			addPickSetItem(Pick::Set&,
					      const SceneID& =SceneID::udf());
    VisID			addRandomLineItem(const RandomLineID&,
					      const SceneID& =SceneID::udf());
    VisID			addWellItem(const MultiID&,
					      const SceneID& =SceneID::udf());
    VisID			add2DLineItem(const Pos::GeomID&,
					      const SceneID& =SceneID::udf());
    VisID			add2DLineItem(const MultiID&,
					      const SceneID& =SceneID::udf());
    VisID			add2DLineItem(const Pos::GeomID&,const SceneID&,
					      bool withdata);
    VisID			addInlCrlItem(OD::SliceType,int nr,
					      const SceneID& =SceneID::udf());
    VisID			addZSliceItem(const TrcKeyZSampling&,
					      const SceneID& =SceneID::udf());
    VisID			addZSliceItem(const TrcKeyZSampling&,
					      const Attrib::SelSpec&,
					      const SceneID& =SceneID::udf());
    VisID			addZSliceItem(RegularSeisDataPack&,
					      const Attrib::SelSpec&,
					      const FlatView::DataDispPars::VD&,
					      const SceneID& =SceneID::udf());
    VisID			addDisplayTreeItem(uiODDisplayTreeItem*,
					      const SceneID& =SceneID::udf());

    void			removeTreeItem(const VisID&);
    uiTreeItem*			findSceneItem(const SceneID&);
    uiTreeItem*			findItem(const VisID&);
    uiTreeItem*			findItem(const VisID&,int attrib);
    void			findItems(const char*,ObjectSet<uiTreeItem>&);
    void			findItems(const char*,ObjectSet<uiTreeItem>&,
					  const SceneID&);

    uiTreeFactorySet*		treeItemFactorySet()	{ return tifs_; }
    uiTreeView*			getTree(const SceneID&);
    void			showTree(bool yn);
    bool			treeShown() const;

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }
    void			setViewSelectMode(int);
    void			setViewSelectMode(const SceneID&,
						  ui3DViewer::PlaneType);

    float			getHeadOnLightIntensity(int) const;
    void			setHeadOnLightIntensity(int,float);

    void			translateText();

    static uiString		sElements()	{ return tr("Elements"); }

protected:

				uiODSceneMgr(uiODMain*);
				~uiODSceneMgr();
    void			initMenuMgrDepObjs();

    void			afterFinalize(CallBacker*);

    uiODMain&			appl_;
    uiMdiArea*			mdiarea_;
    void			mdiAreaChanged(CallBacker*);
    void			sceneChanged(SceneID);

    int				vwridx_;
    uiTreeFactorySet*		tifs_;
    uiWindowGrabber*		wingrabber_;

    inline uiODApplMgr&		applMgr()	{ return appl_.applMgr(); }
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
    Scene*			getScene(const SceneID&);
    const Scene*		getScene(const SceneID&) const;
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

    void			resetStatusBar(const VisID& =VisID::udf());
    void			showIfMinimized(CallBacker*);

public:
    static bool			canAddSceneAtStartup();
};

/*! Settings Tab for mouse interaction. */

mExpClass(uiODMain) uiKeyBindingSettingsGroup : public uiSettingsGroup
{ mODTextTranslationClass(uiKeyBindingSettingsGroup);
public:
    mDefaultFactoryInstantiation2Param( uiSettingsGroup,
					uiKeyBindingSettingsGroup,
					uiParent*,Settings&,
					"Mouse interaction",
					uiStrings::sMouseInteraction());

protected:
			uiKeyBindingSettingsGroup(uiParent*,Settings&);
			~uiKeyBindingSettingsGroup();

private:
    bool		acceptOK() override;
    HelpKey		helpKey() const override;

    uiGenInput*		keybindingfld_;
    uiGenInput*		wheeldirectionfld_;
    uiGenInput*		trackpadzoomspeedfld_;

    BufferString	initialkeybinding_;
    float		initialzoomfactor_;
    bool		initialmousewheelreversal_;
};

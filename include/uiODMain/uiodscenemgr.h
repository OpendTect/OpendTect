#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodapplmgr.h"

#include "datapack.h"
#include "flatview.h"
#include "odpresentationmgr.h"
#include "uivispartserv.h"

class BufferStringSet;
class Timer;
class uiDockWin;
class uiTreeView;
class uiMdiArea;
class uiMdiAreaWindow;
class uiODDisplayTreeItem;
class uiODSceneTreeTop;
class ui3DViewer;
class uiTreeFactorySet;
class uiTreeItem;
class uiWindowGrabber;
class ZAxisTransform;
namespace Pick { class Set; }
namespace Geometry { class RandomLineSet; }
using Presentation::ViewerTypeID;

#define mSceneViewerTypeID ViewerTypeID::get(0)


/*!\brief Manages the scenes and the corresponding trees.

  Most of the interface is really not useful for plugin builders.

 */

mExpClass(uiODMain) uiODScene : public Presentation::ManagedViewer
{
public:
				uiODScene(uiMdiArea*);
				~uiODScene();

private:

    uiDockWin*			dw_;
    uiTreeView*			lv_;
    uiMdiAreaWindow*		mdiwin_;
    ui3DViewer*			vwr3d_;
    uiODSceneTreeTop*		itemmanager_;

    virtual ViewerTypeID	vwrTypeID() const
				{ return mSceneViewerTypeID; }

    friend class		uiODSceneMgr;

};


mExpClass(uiODMain) uiODSceneMgr : public Presentation::VwrTypeMgr
{ mODTextTranslationClass(uiODSceneMgr)
public:

    ViewerTypeID		viewerTypeID()	const
				{ return mSceneViewerTypeID; }
    static ViewerTypeID		theViewerTypeID()
				{ return mSceneViewerTypeID; }

    void			cleanUp(bool startnew=true);
    int				nrScenes()	{ return viewers_.size(); }
    int				addScene(bool maximized,ZAxisTransform* =0,
				    const uiString& nm=uiString::empty());
				//!<Returns scene id
    void			setSceneName(int sceneid,const uiString&);
    uiString			getSceneName(int sceneid) const;
    const ZDomain::Info*	zDomainInfo(int sceneid) const;

    CNotifier<uiODSceneMgr,int>	sceneClosed;
    CNotifier<uiODSceneMgr,int>	treeToBeAdded;
    CNotifier<uiODSceneMgr,int> treeAdded;

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

    int				askSelectScene(const char* zdomfilt=0) const;
				// returns sceneid

    const ui3DViewer*		get3DViewer(int sceneid) const;
    ui3DViewer*			get3DViewer(int sceneid);
    void			get3DViewers(ObjectSet<ui3DViewer>&);
    void			getSceneNames(uiStringSet&,int& act) const;
    void			setActiveScene(int idx);
    void			getActiveSceneName(BufferString&) const;
    int				getActiveSceneID() const;
    Notifier<uiODSceneMgr>	activeSceneChanged;

    uiODSceneTreeTop*		getTreeItemMgr(const uiTreeView*) const;

    void			updateTrees();
    void			rebuildTrees();
    void			setItemInfo(int visid);
    void			updateSelectedTreeItem();
    void			updateItemToolbar(int visid);
    int				getIDFromName(const char*) const;
    void			disabRightClick( bool yn );
    void			disabTrees( bool yn );

    void			getLoadedPickSetIDs(DBKeySet&,bool poly,
						    int scnid=-1) const;
    void			getLoadedEMIDs(DBKeySet&,
					       const char* emtypestr=0,
					       int sceneid=-1) const;
				// if sceneid==-1, then all scenes
    int				addEMItem(const DBKey&,int sceneid=-1);
    int				addPickSetItem(const DBKey&,int sceneid=-1);
    int				addPickSetItem(Pick::Set&,int sceneid=-1);
    int				addRandomLineItem(int rlid,int sceneid=-1);
    int				addWellItem(const DBKey&,int sceneid=-1);
    int				add2DLineItem(Pos::GeomID,int sceneid=-1);
    int				add2DLineItem(const DBKey&,int sceneid=-1);
    int				addInlCrlItem(OD::SliceType,int nr,
					      int sceneid=-1);
    int				addZSliceItem(float z,int sceneid=-1);
    int				addDisplayTreeItem(uiODDisplayTreeItem*,
						   int sceneid=-1);

    void			removeTreeItem(int displayid);
    uiTreeItem*			findItem(int displayid);
    void			findItems(const char*,ObjectSet<uiTreeItem>&);

    uiTreeFactorySet*		treeItemFactorySet()	{ return tifs_; }
    uiTreeView*			getTree(int sceneid);

    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }
    void			setViewSelectMode(int);

    float			getHeadOnLightIntensity(int) const;
    void			setHeadOnLightIntensity(int,float);

    void			translateText();

    static uiString		sElements()	{ return tr("Elements"); }

    uiODScene*			getScene(int sceneid);
    const uiODScene*		getScene(int sceneid) const;

protected:

				uiODSceneMgr(uiODMain*);
				~uiODSceneMgr();
    void			initMenuMgrDepObjs();
    void			resetStatusBar(int id=-1);

    void			afterFinalise(CallBacker*);

    uiODMain&			appl_;
    uiMdiArea*			mdiarea_;
    void			mdiAreaChanged(CallBacker*);

    int				vwridx_;
    uiTreeFactorySet*		tifs_;
    uiWindowGrabber*		wingrabber_;

    inline uiODApplMgr&		applMgr()     { return appl_.applMgr(); }
    inline uiVisPartServer&	visServ()     { return *applMgr().visServer(); }


    uiODScene&			mkNewScene();
    void			removeScene(uiODScene& scene);
    void			removeSceneCB(CallBacker*);
    void			initTree(uiODScene&,int);
    uiODScene*			getSceneByIdx(int idx);
    const uiODScene*		getSceneByIdx(int idx) const;
    void			newSceneUpdated(CallBacker*);
    void			gtLoadedEMIDs(const uiODScene*,
					      DBKeySet&,
					      const char* emtypestr) const;
    void			gtLoadedEMIDs(const uiTreeItem*,
					      DBKeySet&,
					      const char* emtypestr) const;
    void			gtLoadedPickSetIDs(const uiODScene&,
				    DBKeySet&, bool poly) const;
    void			gtLoadedPickSetIDs(const uiTreeItem&,
				    DBKeySet&,bool poly) const;

    Timer*			tiletimer_;
    void			tileTimerCB(CallBacker*);

    void			font3DChanged(CallBacker*);

    friend class		uiODMain;
};

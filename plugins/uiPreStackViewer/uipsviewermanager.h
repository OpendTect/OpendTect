#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2007
________________________________________________________________________

-*/

#include "callback.h"
#include "datapack.h"
#include "menuhandler.h"
#include "multiid.h"
#include "uistring.h"

class uiVisPartServer;
class uiFlatViewMainWin;
class uiMenuHandler;
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{

class uiStoredViewer2DMainWin;
class uiViewer3DPositionDlg;
class uiViewer3DSettingDlg;

/*!Manages prestack data displays in 2D (panel) and 3D (visualization). The
   data itself can be from either lines or volumes. */

mClass(uiPreStackViewer) uiViewer3DMgr : public CallBacker
{ mODTextTranslationClass(uiViewer3DMgr);
public:
				uiViewer3DMgr();
				~uiViewer3DMgr();

    ObjectSet<visSurvey::PreStackDisplay>
				get3DViewers()  { return viewers3d_; }

    //For session
    static const char*		sKeyViewerPrefix()  { return "Viewer "; }
    static const char*		sKey2DViewers()	    { return "PS2DViewers";}
    static const char*		sKeyNrWindows()	    { return "Nr Viewers"; }
    static const char*		sKeyMultiID()	    { return "Data ID"; }
    static const char*		sKeyBinID()	    { return "Position"; }
    static const char*		sKeyIsVolumeData()  { return "Is Volume Data"; }
    static const char*		sKeyTraceNr()	    { return "TraceNr"; }
    static const char*		sKeyLineName()	    { return "LineName"; }

    //For settings
    static const char*		sSettings3DKey()    { return "3DPSViewer"; }
    static const char*		sKeyFlatviewPars()  { return "Flatview"; }

protected:

    uiStoredViewer2DMainWin*	createMultiGather2DViewer(
					const visSurvey::PreStackDisplay&);
    uiFlatViewMainWin*	create2DViewer(const uiString&,DataPackID dpid);

    SceneID		getSceneID(VisID) const;
    static void		getSeis2DTitle(int trnr,const char*,uiString&);
    static void		getSeis3DTitle(const BinID&,const char*,uiString&);
    bool		add3DViewer(const uiMenuHandler*,VisID scnid,
				    int mnuidx);
    void		removeViewWin(DataPackID dpid);
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    uiViewer3DPositionDlg* mkNewPosDialog(const uiMenuHandler*,
					  visSurvey::PreStackDisplay&);

    void		removeAllCB(CallBacker*);
    void		sceneChangeCB(CallBacker*);
    void		surveyToBeChangedCB(CallBacker*);
    void		sessionRestoreCB(CallBacker*);
    void		sessionSaveCB(CallBacker*);
    void		viewer2DSelDataCB(CallBacker*);
    void		viewer2DClosedCB(CallBacker*);
			//Saved 2DViewer for VD only.

    MenuItem		selectpsdatamenuitem_;
    MenuItem		selectpsvwr2ddatamenuitem_;
    MenuItem		proptymenuitem_;
    MenuItem		resolutionmenuitem_;
    MenuItem		positionmenuitem_;
    MenuItem		viewermenuitem_;
    MenuItem		amplspectrumitem_;
    MenuItem		removemenuitem_;

    uiVisPartServer*			visserv_;
    ObjectSet<visSurvey::PreStackDisplay>	viewers3d_;
    ObjectSet<uiViewer3DPositionDlg>	posdialogs_;
    ObjectSet<uiFlatViewMainWin>	viewers2d_;
    ObjectSet<uiViewer3DSettingDlg>	settingdlgs_;
};

} // namespace


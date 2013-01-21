#ifndef psviewermanager_h
#define psviewermanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "callback.h"
#include "menuhandler.h"
#include "multiid.h"

class uiVisPartServer;
class uiFlatViewMainWin;
class uiMenuHandler;
class uiViewer3DPositionDlg;
namespace PreStack { class ProcessManager; }
namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{

class uiStoredViewer2DMainWin;
class uiViewer3DPositionDlg;

/*!Manages pre-stack data displays in 2D (panel) and 3D (visualization). The 
   data itself can be from either lines or volumes. */

class uiViewer3DMgr : public CallBacker
{
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
    static const char*		sKeyBinID()  	    { return "Position"; } 
    static const char*		sKeyIsVolumeData()  { return "Is Volume Data"; }
    static const char*		sKeyTraceNr()	    { return "TraceNr"; } 
    static const char*		sKeyLineName()	    { return "LineName"; } 

    //For settings
    static const char*		sSettings3DKey()    { return "3DPSViewer"; }
    static const char*		sKeyFlatviewPars()  { return "Flatview"; }

protected:
    
    uiStoredViewer2DMainWin*	createMultiGather2DViewer(
					const visSurvey::PreStackDisplay&);
    uiFlatViewMainWin*	create2DViewer(const BufferString&,int dpid);

    int			getSceneID(int mnid);
    static void		getSeis2DTitle(int trnr,const char*,BufferString&);
    static void		getSeis3DTitle(const BinID&,const char*,BufferString&);
    bool		add3DViewer(const uiMenuHandler*,int scnid,int mnuidx);
    void		removeViewWin(int dpid);
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
    PreStack::ProcessManager*		preprocmgr_;    
    ObjectSet<visSurvey::PreStackDisplay>	viewers3d_;
    ObjectSet<uiViewer3DPositionDlg>	posdialogs_;
    ObjectSet<uiFlatViewMainWin>	viewers2d_;
    ObjectSet<uiStoredViewer2DMainWin>	multiviewers2d_;
};

} // namespace

#endif

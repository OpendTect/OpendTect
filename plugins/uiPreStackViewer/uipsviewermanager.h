#ifndef psviewermanager_h
#define psviewermanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2007
 RCS:		$Id: uipsviewermanager.h,v 1.12 2008-12-22 19:25:37 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "menuhandler.h"
#include "multiid.h"

class uiVisPartServer;
class uiFlatViewWin;
class uiMenuHandler;
namespace PreStack { class ProcessManager; }

namespace PreStackView
{

class Viewer3D;
class uiViewer3DPositionDlg;

/*!Manages pre-stack data displays in 2D (panel) and 3D (visualization). The 
   data itself can be from either lines or volumes. */

class uiViewer3DMgr : public CallBacker
{
public:
				uiViewer3DMgr();
				~uiViewer3DMgr();

    ObjectSet<PreStackView::Viewer3D>   get3DViewers()  { return viewers3d_; }

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
    
    uiFlatViewWin*	create2DViewer(const BufferString&,int datapackid);
    int			getSceneID(int mnid);
    static void		getSeis2DTitle(int trnr,const char*,BufferString&);
    static void		getSeis3DTitle(const BinID&,const char*,BufferString&);
    bool		add3DViewer(const uiMenuHandler*,int sceneid,int mnuidx);
    void		removeViewWin(const int dpid);
    void		createMenuCB(CallBacker*);
    void		handleMenuCB(CallBacker*);
    void		removeAllCB(CallBacker*);
    void		sceneChangeCB(CallBacker*);
    void		surveyToBeChangedCB(CallBacker*);
    void		sessionRestoreCB(CallBacker*);
    void		sessionSaveCB(CallBacker*);
    				//Saved 2DViewer for VD only.

    MenuItem		selectpsdatamenuitem_;
    MenuItem		proptymenuitem_;
    MenuItem		positionmenuitem_;
    MenuItem		viewermenuitem_;
    MenuItem		amplspectrumitem_;
    MenuItem		removemenuitem_;

    uiVisPartServer*			visserv_;
    PreStack::ProcessManager*		preprocmgr_;    
    ObjectSet<PreStackView::Viewer3D>	viewers3d_;
    ObjectSet<uiViewer3DPositionDlg>	posdialogs_;
    ObjectSet<uiFlatViewWin>		viewers2d_;
};

}; //namespace

#endif

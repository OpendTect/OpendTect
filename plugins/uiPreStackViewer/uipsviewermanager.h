#ifndef psviewermanager_h
#define psviewermanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2007
 RCS:		$Id: uipsviewermanager.h,v 1.10 2008-12-19 21:58:00 cvsyuancheng Exp $
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

class Viewer;
class uiViewerPositionDlg;

/*!Manages psviewers in the 3d visualization. */
class uiViewerMgr : public CallBacker
{
public:
				uiViewerMgr();
				~uiViewerMgr();

    ObjectSet<PreStackView::Viewer>   getViewers()	{ return viewers_; }

    //For session
    static const char*		sKeyViewerPrefix()	{ return "Viewer "; } 
    static const char*		sKey2DViewers()		{ return "PS2DViewers";}
    static const char*		sKeyNrWindows()		{ return "Nr Viewers"; }
    static const char*		sKeyMultiID()		{ return "Data ID"; }
    static const char*		sKeyBinID()  		{ return "Position"; } 
    static const char*		sKeyIs3D()   		{ return "Is 3D"; } 
    static const char*		sKeyTraceNr()		{ return "TraceNr"; } 
    static const char*		sKeyLineName()		{ return "LineName"; } 

    //For settings
    static const char*		sSettingsKey()		{ return "3DPSViewer"; }
    static const char*		sKeyFlatviewPars()	{ return "Flatview"; }

protected:
    
    uiFlatViewWin*		create2DViewer(BufferString,int datapackid);
    int				getSceneID(int mnid);
    const char*			getSeis2DTitle(const int trnr,BufferString);
    const char*			getSeis3DTitle(BinID,BufferString);
    bool			addNewPSViewer(const uiMenuHandler*,
	    				       int sceneid,int mnuidx);
    void			removeViewWin(const int dpid);
    void			createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    void			removeAllCB(CallBacker*);
    void			sceneChangeCB(CallBacker*);
    void			surveyToBeChangedCB(CallBacker*);
    void			sessionRestoreCB(CallBacker*);
    void			sessionSaveCB(CallBacker*);
    				//Saved 2DViewer for VD only.

    MenuItem			selectpsdatamenuitem_;
    MenuItem			proptymenuitem_;
    MenuItem			positionmenuitem_;
    MenuItem			viewermenuitem_;
    MenuItem			amplspectrumitem_;
    MenuItem			removemenuitem_;

    PreStack::ProcessManager*   preprocmgr_;    
    uiVisPartServer*		visserv_;

    ObjectSet<PreStackView::Viewer>	viewers_;
    ObjectSet<uiViewerPositionDlg>	posdialogs_;

    ObjectSet<uiFlatViewWin>	viewwindows_;
};

}; //namespace

#endif

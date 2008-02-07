#ifndef psviewermanager_h
#define psviewermanager_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2007
 RCS:		$Id: uipsviewermanager.h,v 1.5 2008-02-07 14:30:18 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "callback.h"
#include "menuhandler.h"
#include "multiid.h"

class uiVisPartServer;
class uiFlatViewWin;
class uiMenuHandler;

namespace PreStackView
{

class PreStackViewer;

/*!Manages psviewers in the 3d visualization. */
class uiPSViewerMgr : public CallBacker
{
public:
				uiPSViewerMgr();
				~uiPSViewerMgr();

    ObjectSet<PreStackViewer>   getViewers()  { return viewers_; }

    static const char*		sKeyViewerPrefix() { return "Viewer "; } 
    static const char*		sKey2DViewers() {return "PreStack 2D Viewers";} 
    static const char*		sKeyNrWindows() { return "Number of windows"; } 
    static const char*		sKeyMultiID() {return "uiFlatViewWin MultiID";}
    static const char*		sKeyBinID()   { return "uiFlatViewWin binid"; } 
    static const char*		sKeyIs3D()    { return "Seis3D display"; } 
    static const char*		sKeyTraceNr() { return "Seis2D TraceNr"; } 
    static const char*		sKeySeis2DName() { return "Seis2D Name"; } 

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
    MenuItem			removemenuitem_;
    MenuItem			proptymenuitem_;
    MenuItem			viewermenuitem_;
    
    uiVisPartServer*		visserv_;
    ObjectSet<PreStackViewer>	viewers_;
    ObjectSet<uiFlatViewWin>	viewwindows_;
};

}; //namespace

#endif

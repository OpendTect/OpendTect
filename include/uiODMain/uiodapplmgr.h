#ifndef uiodapplmgr_h
#define uiodapplmgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiodapplmgr.h,v 1.1 2003-12-20 13:24:05 bert Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"
class uiApplPartServer;
class uiNLAPartServer;
class uiAttribPartServer;
class uiEMPartServer;
class uiPickPartServer;
class uiSeisPartServer;
class uiVisPartServer;
class uiWellPartServer;
class uiODApplService;
class uiApplService;
class Color;
class ODSession;
class MultiID;
class PickSetGroup;
class uidTectMan;
class uiPopupMenu;
class uiSoViewer;

/*!\brief Application level manager - ties part servers together */

class uiODApplMgr
{
public:

			uiODApplMgr(uiODMain&);
			~uiODApplMgr();

    uiPickPartServer*	pickServer()			{ return pickserv; }
    uiVisPartServer*	visServer()			{ return visserv; }
    uiSeisPartServer*	seisServer()			{ return seisserv; }
    uiAttribPartServer* attrServer()			{ return attrserv; }
    uiEMPartServer*	EMServer() 			{ return emserv; }
    uiWellPartServer*	wellServer()			{ return wellserv; }
    uiNLAPartServer*	nlaServer()			{ return nlaserv; }
    void		setNlaServer( uiNLAPartServer* s ) { nlaserv = s; }

    void		doOperation(uiODMain::ObjType,
	    			    uiODMain::ActType,int opt=0);
    			//!< Not all combinations are available ...!

    // File menu operations
    int			manageSurvey();
    bool		manageNLA();
    void		importPickSet();
    void		importLMKFault();

    // Processing menu operations
    void		createVol();
    void		reStartProc();

    // Processing menu operations
    void		batchProgs();
    void		pluginMan();
    void		crDevEnv();
    void		doHelp(const char*,const char*);
    void		setFonts();

    bool		storePickSets();
    bool		storeSinglePickSet(int);
    bool		setPickSetDirs(int);
    void		getPickSetGroup(PickSetGroup& psg);
    const Color&	getPickColor();
    void		renamePickset(int);

    bool		selectAttrib(int);
    bool		getNewData(int visid,bool);
    bool		evaluateAttribute(int visid);

    bool		selectColorAttrib(int);

    void		saverestoreSession(bool);
    bool		hasSessionChanged();
    bool		saveSession(ODSession*);
    void		restoreSession(ODSession*);

    void		selectWells(ObjectSet<MultiID>&);

    void		selectHorizon(MultiID&);
    void		selectStickSet(MultiID&);
    void		selectFault(MultiID&);
    void		storeSurface(int);

    bool		createSubMenu(uiPopupMenu&,int mnuid,int visid,int tp);
    bool		handleSubMenu(int mnuid,int visid,int tp);

    uiApplService&	applService();

    Notifier<uiODApplMgr> sessionSave;
    Notifier<uiODApplMgr> sessionRestore;
    IOPar&		sessionPar()		{ return *pluginsessionpars; }

    void		prepClose(CallBacker*);
    void		sceneRemoved(int,uiDockWin*);
    void		setStereoOffset(ObjectSet<uiSoViewer>&);

protected:

    uiODMain&		appl;
    uiODApplService&	applservice;

    uiPickPartServer*	pickserv;
    uiVisPartServer*	visserv;
    uiNLAPartServer*	nlaserv;
    uiAttribPartServer* attrserv;
    uiSeisPartServer*	seisserv;
    uiEMPartServer*	emserv;
    uiWellPartServer*	wellserv;
    ODSession*		cursession;

    IOPar*		pluginsessionpars;

    void		handleStoredSurfaceData(int);

    bool		handleEvent(const uiApplPartServer*,int);
    void*		deliverObject(const uiApplPartServer*,int);

    bool		getNewCubeData(int visid,bool);
    bool		getNewRandomLineData(int,bool);
    bool		getNewSurfData(int,bool);

    bool		handlePickServEv(int);
    bool		handleVisServEv(int);
    bool		handleNLAServEv(int);
    bool		handleAttribServEv(int);
    bool		handleEMServEv(int);
    bool		handleWellServEv(int);

    bool		attrdlgisup;

    friend class	uiODApplService;
};


#endif

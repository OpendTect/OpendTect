#ifndef uiodapplmgr_h
#define uiodapplmgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiodapplmgr.h,v 1.2 2003-12-24 15:15:42 bert Exp $
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


    // File menu operations
    int			manageSurvey();
    bool		manageNLA();
    void		doOperation(uiODMain::ObjType,
	    			    uiODMain::ActType,int opt=0);
    			//!< Not all combinations are available ...!
    void		importPickSet();
    void		importLMKFault();

    // Processing menu operations
    void		manageAttributes();
    void		createVol();
    void		reStartProc();

    // Utility menu operations
    void		batchProgs();
    void		pluginMan();
    void		crDevEnv();
    void		doHelp(const char*,const char*);
    void		setFonts();
    void		setStereoOffset();

    // Tree menu services
	// Selections
    void		selectWells(ObjectSet<MultiID>&);
    void		selectHorizon(MultiID&);
    void		selectFault(MultiID&);
    void		selectStickSet(MultiID&);
    bool		selectAttrib(int);
    bool		selectColorAttrib(int);
	// Surfaces
    void		storeSurface(int);
	// PickSets
    const Color&	getPickColor();
    void		getPickSetGroup(PickSetGroup& psg);
    bool		storePickSets();
    bool		storeSinglePickSet(int);
    bool		setPickSetDirs(int);
    void		renamePickset(int);
	// Menu
    bool		createSubMenu(uiPopupMenu&,int mnuid,int visid,int tp);
    bool		handleSubMenu(int mnuid,int visid,int tp);

    // Work
    bool		getNewData(int visid,bool);
    bool		evaluateAttribute(int visid);

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

    bool		handleEvent(const uiApplPartServer*,int);
    void*		deliverObject(const uiApplPartServer*,int);

    bool		handleWellServEv(int);
    bool		handleEMServEv(int);
    bool		handlePickServEv(int);
    bool		handleVisServEv(int);
    bool		handleNLAServEv(int);
    bool		handleAttribServEv(int);

    bool		getNewCubeData(int visid,bool);
    bool		getNewSurfData(int,bool);
    bool		getNewRandomLineData(int,bool);
    void		handleStoredSurfaceData(int);

    void		modifyColorTable(int);
    void		setHistogram(int);

    friend class	uiODApplService;

    inline uiODSceneMgr& sceneMgr()	{ return appl.sceneMgr(); }
    inline uiODMenuMgr&	menuMgr()	{ return appl.menuMgr(); }
};


#endif

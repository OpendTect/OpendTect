#ifndef uiodapplmgr_h
#define uiodapplmgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uiodapplmgr.h,v 1.126 2012-08-03 13:01:03 cvskris Exp $
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodmain.h"

class MouseCursorExchange;
class uiApplPartServer;
class uiApplService;
class uiAttribPartServer;
class uiEMAttribPartServer;
class uiEMPartServer;
class uiMPEPartServer;
class uiNLAPartServer;
class uiODApplService;
class uiPopupMenu;
class uiPickPartServer;
class uiSeisPartServer;
class uiStratTreeWin;
class uiTreeItem;
class uiVisPartServer;
class uiWellAttribPartServer;
class uiWellPartServer;
class uiODApplMgrDispatcher;
class uiODApplMgrAttrVisHandler;
class uiVisDataPointSetDisplayMgr;

class Color;
class Coord;
class DataPointSet;
class MultiID;
class ODSession;
namespace Attrib { class SelSpec; }
namespace Pick { class Set; }

/*!\brief Application level manager - ties part servers together

  The uiODApplMgr instance can be accessed like:
  ODMainWin()->applMgr()
  For plugins it may be interesting to pop up one of the OpendTect standard
  dialogs at any point in time. The best way to do that is by calling one
  of the uiODApplMgr methods.

  A big part of this class is dedicated to handling the events from the various
  part servers.
 
 */

mClass(uiODMain) uiODApplMgr : public CallBacker
{
public:

    uiPickPartServer*		pickServer()		{ return pickserv_; }
    uiVisPartServer*		visServer()		{ return visserv_; }
    uiSeisPartServer*		seisServer()		{ return seisserv_; }
    uiAttribPartServer*		attrServer()		{ return attrserv_; }
    uiEMPartServer*		EMServer() 		{ return emserv_; }
    uiEMAttribPartServer*	EMAttribServer()	{ return emattrserv_; }
    uiWellPartServer*		wellServer()		{ return wellserv_; }
    uiWellAttribPartServer*	wellAttribServer()	{ return wellattrserv_;}
    uiMPEPartServer*		mpeServer()		{ return mpeserv_; }
    uiNLAPartServer*		nlaServer()		{ return nlaserv_; }
    void			setNlaServer(uiNLAPartServer* s);
    uiApplService&		applService()
				{ return (uiApplService&)applservice_; }


    // Survey menu operations
    int				manageSurvey();
    enum ObjType		{ Seis, Hor, Flt, Wll, Attr, NLA, Pick, Sess,
				  Strat, Wvlt, MDef, Vel, PDF, PVDS, Geom, 
				  Body, Props };
    enum ActType		{ Imp, Exp, Man };
    void			doOperation(ObjType,ActType,int opt=0);
    				//!< Not all combinations are available ...!
    void			manPreLoad(ObjType);

    // Processing menu operations
    void			editAttribSet();
    void			editAttribSet(bool);
    bool			editNLA(bool);
    void			createVol(bool);
    void			doWellXPlot(CallBacker* =0);
    				//!< This plots between well and attrib
    void			doAttribXPlot(CallBacker* =0);
    				//!< This plots between attribs.
    void			openCrossPlot(CallBacker* = 0 ); // Crate XPlot from file
    void			createHorOutput(int,bool);
    void			reStartProc();
    void			processTime2Depth(CallBacker* =0);
    void			processPreStack(CallBacker* =0);
    void			processVelConv(CallBacker* =0);
    void			genAngleMuteFunction(CallBacker* =0);
    void			bayesClass2D(CallBacker* =0);
    void			bayesClass3D(CallBacker* =0);
    void			resortSEGY(CallBacker* =0);
    void			createCubeFromWells(CallBacker* =0);
    void			create2Dfrom3D();
    void			create3Dfrom2D();

    // View menu operations
    void			showBaseMap();
    void			setWorkingArea();
    void			setZStretch();
    void			setStereoOffset();

    // Scene menu operations
    void			addTimeDepthScene();

    // Utility menu operations
    void			batchProgs();
    void			pluginMan();
    void			posConversion();
    void			crDevEnv();
    void			setFonts();
    void			manageShortcuts();
    void			startInstMgr();
    void			setAutoUpdatePol();

    // Tree menu services
    // Selections
    void			selectWells(ObjectSet<MultiID>&);
    void			selectHorizon(MultiID&);
    void			selectFault(MultiID&);
    void			selectPolygonSurface(MultiID&);
    void			selectStickSet(MultiID&);
    bool			selectAttrib(int id,int attrib);

    // PickSets
    bool			storePickSets();
    bool			storePickSet(const Pick::Set&);
    bool			storePickSetAs(const Pick::Set&);
    bool			setPickSetDirs(Pick::Set&);
    bool			pickSetsStored() const;

    // Tool to exhange mouse-cursor information between windows
    MouseCursorExchange&	mouseCursorExchange();

    // Work. Don't use unless expert.
    uiVisDataPointSetDisplayMgr* visDPSDispMgr()
    				{ return visdpsdispmgr_; }
    bool			getNewData(int visid,int attrib);
    bool			evaluateAttribute(int visid,int attrib);
    bool			evaluate2DAttribute(int visid, int attrib);
    void			pageUpDownPressed(bool up);
    void			resetServers();
    void			updateColorTable(int visid,int attrib);
    void			saveDefColTab(int visid,int attrib);
    void			calShiftAttribute(int attrib,
	                                        const Attrib::SelSpec& as );
    bool			calcRandomPosAttrib(int visid,int attrib);
    bool			calcMultipleAttribs(Attrib::SelSpec&);
    NotifierAccess*		colorTableSeqChange();
    void			addVisDPSChild(CallBacker*);
    void			manSurvCB(CallBacker*)	  { manageSurvey(); }
    void			seisOut2DCB(CallBacker*)  { createVol(true); }
    void			seisOut3DCB(CallBacker*)  { createVol(false); }
    void			createVolProcOutput(CallBacker*);
    void			editAttr2DCB(CallBacker*)
				    { editAttribSet(true); }
    void			editAttr3DCB(CallBacker*)
				    { editAttribSet(false);}
    Notifier<uiODApplMgr>	attribSetChg;

    void			doVolProcCB(CallBacker*);
    void			doVolProc(const MultiID&);
    void			tieWellToSeismic(CallBacker*);
    void			doWellLogTools(CallBacker*);
    void			doLayerModeling(CallBacker*);
    void			setupRdmLinePreview(const TypeSet<Coord>&);
    void			cleanPreview();

    inline uiODMenuMgr&		menuMgr()	{ return appl_.menuMgr(); }
    void			enableMenusAndToolBars(bool);
    void			enableTree(bool);
    void			enableSceneManipulation(bool);
    				/*!<Turns on/off viewMode and enables/disables
				    the possibility to go to actMode. */

    Notifier<uiODApplMgr>	getOtherFormatData;
				/*!<Is triggered when the vispartserver wants
				    data, but the format (as reported by
				    uiVisPartServer::getAttributeFormat() ) is
				    uiVisPartServer::OtherFormat. If you manage
				    a visobject with OtherFormat, you can
				    connect here to be notified when the object
				    needs data. The visid and attribidx is
				    retrieved by otherFormatVisID and
				    otherFormatAttrib. */
    int				otherFormatVisID() const
				    { return otherformatvisid_; }
    int				otherFormatAttrib() const
				    { return otherformatattrib_; }
    int				createMapDataPack(const DataPointSet&,int col);
    void			useDefColTab(int visid,int attrib);

protected:

				uiODApplMgr(uiODMain&);
				~uiODApplMgr();

    uiODMain&			appl_;
    uiODApplService&		applservice_;

    uiPickPartServer*		pickserv_;
    uiVisPartServer*		visserv_;
    uiNLAPartServer*		nlaserv_;
    uiAttribPartServer*		attrserv_;
    uiSeisPartServer*		seisserv_;
    uiEMPartServer*		emserv_;
    uiEMAttribPartServer*	emattrserv_;
    uiWellPartServer*		wellserv_;
    uiWellAttribPartServer*	wellattrserv_;
    uiMPEPartServer*		mpeserv_;
    uiODApplMgrDispatcher&	dispatcher_;
    uiODApplMgrAttrVisHandler&	attrvishandler_;
    MouseCursorExchange&	mousecursorexchange_;

    struct MiscSurvInfo
    {
				MiscSurvInfo()
				: xyunit_(1)
			        , zunit_(0)
				, zstep_(0.004) {};

	void                    refresh();

	int			xyunit_;
	int			zunit_;
	float			zstep_;
    };

    MiscSurvInfo		tmpprevsurvinfo_;
    bool			survChgReqAttrUpdate();

    bool			handleEvent(const uiApplPartServer*,int);
    void*			deliverObject(const uiApplPartServer*,int);

    bool			handleMPEServEv(int);
    bool			handleWellServEv(int);
    bool			handleWellAttribServEv(int);
    bool			handleEMServEv(int);
    bool			handleEMAttribServEv(int);
    bool			handlePickServEv(int);
    bool			handleVisServEv(int);
    bool			handleNLAServEv(int);
    bool			handleAttribServEv(int);

    void			surveyToBeChanged(CallBacker*);
    void			surveyChanged(CallBacker*);
    void			colSeqChg(CallBacker*);
    void			colMapperChg(CallBacker*);
    void			setHistogram(int visid,int attrib);
    void			storeEMObject();

    void			manStrat();

    void			createAndSetMapDataPack(int visid,int attrib,
	    					const DataPointSet&,int colnr);

    friend class		uiODApplService;

    inline uiODSceneMgr&	sceneMgr()	{ return appl_.sceneMgr(); }

    int				otherformatvisid_;
    int				otherformatattrib_;

    uiVisDataPointSetDisplayMgr* visdpsdispmgr_;

    friend class		uiODMain;
    friend class		uiODApplMgrDispatcher;
    friend class		uiODApplMgrAttrVisHandler;
};


#endif


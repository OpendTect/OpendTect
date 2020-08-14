#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2001
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
class uiPickPartServer;
class uiSeisPartServer;
class uiTreeItem;
class uiVisPartServer;
class uiVolProcPartServer;
class uiWellAttribPartServer;
class uiWellPartServer;
class uiODApplMgrDispatcher;
class uiODApplMgrAttrVisHandler;
class uiVisDataPointSetDisplayMgr;

class DataPointSet;
class ODSession;
namespace Attrib { class DescID; class SelSpec; }
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

mExpClass(uiODMain) uiODApplMgr : public CallBacker
{ mODTextTranslationClass(uiODApplMgr);
public:

    uiPickPartServer*		pickServer()		{ return pickserv_; }
    uiVisPartServer*		visServer()		{ return visserv_; }
    uiSeisPartServer*		seisServer()		{ return seisserv_; }
    uiAttribPartServer*		attrServer()		{ return attrserv_; }
    uiVolProcPartServer*	volprocServer()		{ return volprocserv_; }
    uiEMPartServer*		EMServer()		{ return emserv_; }
    uiEMAttribPartServer*	EMAttribServer()	{ return emattrserv_; }
    uiWellPartServer*		wellServer()		{ return wellserv_; }
    uiWellAttribPartServer*	wellAttribServer()	{ return wellattrserv_;}
    uiMPEPartServer*		mpeServer()		{ return mpeserv_; }
    uiNLAPartServer*		nlaServer()		{ return nlaserv_; }

    const uiPickPartServer*	pickServer() const	{ return pickserv_; }
    const uiVisPartServer*	visServer() const	{ return visserv_; }
    const uiSeisPartServer*	seisServer() const	{ return seisserv_; }
    const uiAttribPartServer*	attrServer() const	{ return attrserv_; }
    const uiVolProcPartServer*	volprocServer() const	{ return volprocserv_; }
    const uiEMPartServer*	EMServer() const	{ return emserv_; }
    const uiEMAttribPartServer*	EMAttribServer() const	{ return emattrserv_; }
    const uiWellPartServer*	wellServer() const	{ return wellserv_; }
    const uiWellAttribPartServer* wellAttribServer() const
							{ return wellattrserv_;}
    const uiMPEPartServer*	mpeServer() const	{ return mpeserv_; }
    const uiNLAPartServer*	nlaServer() const	{ return nlaserv_; }

    void			setNlaServer(uiNLAPartServer* s);
    uiApplService&		applService()
				{ return (uiApplService&)applservice_; }
    inline uiODSceneMgr&	sceneMgr()	{ return appl_.sceneMgr(); }


    // Survey menu operations
    bool			selectSurvey(uiParent*);
    bool			manageSurvey()		{ return manSurv(0); }

    // The order of this enum needs to be exactly as in UI, beacuse
    // uiODMenuMgr::getMnu() depends on it!
    enum ObjType	{ Attr, Body, ColTab, XPlot, Flt, Fltss, FltSet, Geom2D,
			  Hor, Props, MDef, NLA, Pick, Poly, PDF, RanL,
			  Seis, Sess, Strat, Vel, Wvlt, Wll };
    int			nrObjectTypes() const	{ return Wll+1; }

    enum ActType	{ Imp, Exp, Man, PL };
    int			nrActTypes() const	{ return PL+1; }

    void		doOperation(ObjType,ActType,int opt=0);
			//!< Not all combinations are available ...!
    void		manPreLoad(ObjType);

    // Processing menu operations
    void		editAttribSet();
    void		editAttribSet(bool is2d);
    bool		editNLA(bool is2d);
    bool		uvqNLA(bool is2d);
    void		createVol(bool is2d,bool multiattrib);
    void		createVolProcOutput(bool);
    void		doWellXPlot(CallBacker* =0);
			//!< This plots between well and attrib
    void		doAttribXPlot(CallBacker* =0);
			//!< This plots between attribs.
    void		openCrossPlot(CallBacker* =0);
			//!< Create crossplot from file
    void		createHorOutput(int,bool);
    void		startBatchJob();
    void		processTime2Depth(bool is2d);
    void		processPreStack(bool is2d);
    void		createMultiCubeDS(CallBacker* =0);
    void		createMultiAttribVol(CallBacker*);
    void		processVelConv(CallBacker* =0);
    void		genAngleMuteFunction(CallBacker* =0);
    void		bayesClass2D(CallBacker* =0);
    void		bayesClass3D(CallBacker* =0);
    void		createCubeFromWells(CallBacker* =0);
    void		create2DGrid()		{ process2D3D(0); }
    void		create2DFrom3D()	{ process2D3D(1); }
    void		create3DFrom2D()	{ process2D3D(2); }

    // View menu operations
    void		setWorkingArea();
    void		setZStretch();
    void		setStereoOffset();
    void		show2DViewer();

    // Scene menu operations
    void		addTimeDepthScene(bool is2d);
    void		addHorFlatScene(bool is2d);

    // Utility menu operations
    void		batchProgs();
    void		setupBatchHosts();
    void		pluginMan();
    void		posConversion();
    void		crsPosConversion();
    void		crDevEnv();
    void		manageShortcuts();
    void		startInstMgr();
    void		setAutoUpdatePol();

    // Tree menu services
    // Selections
    void		selectWells(DBKeySet&);
    void		selectHorizon(DBKey&);
    void		selectFault(DBKey&);
    void		selectPolygonSurface(DBKey&);
    void		selectStickSet(DBKey&);
    bool		selectAttrib(int id,int attrib);

    // PickSets
    bool		storePickSets(int polyopt,const char* cat=0);
    bool		storePickSet(const Pick::Set&);
    bool		storePickSetAs(const Pick::Set&);
    bool		setPickSetDirs(Pick::Set&);

    // Tool to exchange mouse-cursor information between windows
    MouseCursorExchange& mouseCursorExchange();

    // Work. Don't use unless expert.
    uiVisDataPointSetDisplayMgr* visDPSDispMgr()
			{ return visdpsdispmgr_; }
    inline uiODViewer2DMgr& viewer2DMgr()
			{ return appl_.viewer2DMgr(); }
    bool		getNewData(int visid,int attrib);
    bool		evaluateAttribute(int visid,int attrib);
    bool		evaluate2DAttribute(int visid, int attrib);
    void		pageUpDownPressed(bool up);
    void		handleSurveySelect();
    bool		isFreshSurvey() const;
    void		handleSIPImport();
    void		resetServers();
    void		hideColorTable();
    void		updateColorTable(int visid,int attrib);
    void		saveDefColTab(int visid,int attrib);
    bool		getDefaultDescID(Attrib::DescID&,bool is2d,
					 bool isstored) const;
    void		calcShiftAttribute(int attrib,const Attrib::SelSpec&);
    bool		calcRandomPosAttrib(int visid,int attrib);
    bool		calcMultipleAttribs(Attrib::SelSpec&);
    void		addVisDPSChild(CallBacker*);
    void		manSurvCB(CallBacker*);
    void		seisOut2DCB(CallBacker*);
    void		seisOut3DCB(CallBacker*);
    void		editAttr2DCB(CallBacker*)   { editAttribSet(true); }
    void		editAttr3DCB(CallBacker*)   { editAttribSet(false);}

    Notifier<uiODApplMgr> attribSetChg;

    void		doVolProc2DCB(CallBacker*);
    void		doVolProc3DCB(CallBacker*);
    void		doVolProc(const DBKey&);
    void		tieWellToSeismic(CallBacker*);
    void		doWellLogTools(CallBacker*);
    void		launchRockPhysics(CallBacker*);
    void		launch2DViewer(CallBacker*);
    void		doLayerModeling(CallBacker*);
    void		setupRdmLinePreview(const TypeSet<Coord>&);
    void		cleanPreview();
    void		addMPEParentPath(int visid,const TrcKey&);

    void		enableMenusAndToolBars(bool);
    void		enableTree(bool);
    void		enableSceneManipulation(bool);
				/*!<Turns on/off viewMode and enables/disables
				    the possibility to go to actMode. */

    Notifier<uiODApplMgr> getOtherFormatData;
				/*!<Is triggered when the vispartserver wants
				    data, but the format (as reported by
				    uiVisPartServer::getAttributeFormat() ) is
				    uiVisPartServer::OtherFormat. If you manage
				    a visobject with OtherFormat, you can
				    connect here to be notified when the object
				    needs data. The visid and attribidx is
				    retrieved by otherFormatVisID and
				    otherFormatAttrib. */
    int			otherFormatVisID() const { return otherformatvisid_; }
    int			otherFormatAttrib() const { return otherformatattrib_; }
    void		useDefColTab(int visid,int attrib);
    bool		isRestoringSession() const;

protected:

				uiODApplMgr(uiODMain&);
				~uiODApplMgr();

    uiODMain&			appl_;
    uiODApplService&		applservice_;

    uiPickPartServer*		pickserv_;
    uiVisPartServer*		visserv_;
    uiNLAPartServer*		nlaserv_;
    uiAttribPartServer*		attrserv_;
    uiVolProcPartServer*	volprocserv_;
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
				, zstep_(0.004f) {}

	void			refresh();

	int			xyunit_;
	int			zunit_;
	float			zstep_;
    };

    MiscSurvInfo		tmpprevsurvinfo_;
    bool			manSurv(uiParent*);
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
    bool			handleVolProcServEv(int);

    void			surveyChangeOKCB(CallBacker*);
    void			surveyToBeChanged(CallBacker*);
    void			surveyChanged(CallBacker*);
    void			colSeqChg(CallBacker*);
    void			storeEMObject(bool saveas=false);

    void			manStrat();

    void			setRandomPosData(int visid,int attrib,
						const DataPointSet&);
    void			process2D3D(int opt); /*!<opt=0: create 2D grid,
							  1:3D->2D, 2:2D->3D */

    friend class		uiODApplService;

    int				otherformatvisid_;
    int				otherformatattrib_;

    uiVisDataPointSetDisplayMgr* visdpsdispmgr_;

    static bool			Convert_OD4_Data_To_OD5();
    static bool			Convert_OD4_Body_To_OD5();
    void			mainWinUpCB(CallBacker*);

    friend class		uiODMain;
    friend class		uiODApplMgrDispatcher;
    friend class		uiODApplMgrAttrVisHandler;

};

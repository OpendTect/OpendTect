#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uiapplserv.h"
#include "uiodmain.h"
#include "survinfo.h"

class MouseCursorExchange;
class uiAttribPartServer;
class uiEMAttribPartServer;
class uiEMPartServer;
class uiMPEPartServer;
class uiNLAPartServer;
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

mExpClass(uiODMain) uiODApplMgr : public uiApplMgr
{ mODTextTranslationClass(uiODApplMgr);
public:

    uiEMPartServer*		EMServer() override	{ return emserv_; }
    uiPickPartServer*		pickServer() override	{ return pickserv_; }
    uiSeisPartServer*		seisServer() override	{ return seisserv_; }
    uiWellPartServer*		wellServer() override	{ return wellserv_; }
    uiVisPartServer*		visServer() override	{ return visserv_; }
    uiAttribPartServer*		attrServer() override	{ return attrserv_; }
    uiVolProcPartServer*	volprocServer() override{ return volprocserv_; }
    uiEMAttribPartServer*	EMAttribServer() override{ return emattrserv_; }
    uiWellAttribPartServer*	wellAttribServer() override
							{ return wellattrserv_;}
    uiMPEPartServer*		mpeServer() override	{ return mpeserv_; }
    uiNLAPartServer*		nlaServer() override	{ return nlaserv_; }

    void			setNlaServer(uiNLAPartServer*) override;

    uiVisColTabEd&		colTabEd()	{ return appl_.colTabEd(); }

    // Survey menu operations
    int				selectSurvey(uiParent*);
    int				editCurrentSurvey(uiParent*);
    static int			manageSurvey()		{ return manSurv(0); }
    void			exportSurveySetup();

    enum ObjType		{ Seis, Hor, Flt, Fltss, FltSet, Wll, Attr, NLA,
				  Pick, Sess, Strat, Wvlt, MDef, Vel, PDF, PVDS,
				  Geom, Body, Props, ColTab, RanL, NrObjTypes };
    enum ActType		{ Imp, Exp, Man };
    void			doOperation(ObjType,ActType,int opt=0);
				//!< Not all combinations are available ...!
    void			manPreLoad(ObjType);

    // Processing menu operations
    void			editAttribSet();
    void			editAttribSet(bool is2d);
    bool			editNLA(bool is2d);
    bool			uvqNLA(bool is2d);
    void			saveNN();
    void			createVol(bool is2d,bool multiattrib);
    void			createVolProcOutput(bool);
    void			doWellXPlot(CallBacker* =nullptr);
				//!< This plots between well and attrib
    void			doAttribXPlot(CallBacker* =nullptr);
				//!< This plots between attribs.
    void			openCrossPlot(CallBacker* =nullptr);
				//!< Create crossplot from file
    void			createHorOutput(int,bool);
    void			doStratAmp(CallBacker*);
    void			doIsochron(CallBacker*);
    void			startBatchJob();
    void			processTime2DepthSeis(CallBacker* =nullptr);
    void			processTime2DepthSeis(bool is2d);
    void			processTime2DepthHor(bool is2d);
    void			processTime2DepthFSS(bool is2d);
    void			processPreStack(bool is2d);
    void			createMultiCubeDS(CallBacker* =nullptr);
    void			createMultiAttribVol(CallBacker*);
    void			processVelConv(CallBacker* =nullptr);
    void			genAngleMuteFunction(CallBacker* =nullptr);
    void			bayesClass2D(CallBacker* =nullptr);
    void			bayesClass3D(CallBacker* =nullptr);
    void			createCubeFromWells(CallBacker* =nullptr);
    void			fltTimeDepthConvCB(CallBacker* =nullptr);
    void			fltsetTimeDepthConvCB(CallBacker* =nullptr);
    void			create2DGrid()		{ process2D3D(0); }
    void			create2DFrom3D()	{ process2D3D(1); }
    void			create3DFrom2D()	{ process2D3D(2); }
    void			interpol3DFrom2D()	{ process2D3D(3); }

    // View menu operations
    void			setWorkingArea();
    void			setZStretch();
    void			setStereoOffset();
    void			show2DViewer();

    // Scene menu operations
    void			addTimeDepthScene(bool is2d=false);
    void			addHorFlatScene(bool is2d);

    // Utility menu operations
    void			batchProgs();
    void			setupBatchHosts();
    void			pluginMan();
    void			posConversion();
    void			crDevEnv();
    void			manageShortcuts();
    void			startInstMgr();
    void			setAutoUpdatePol();

    // Tree menu services
    // Selections
    void			selectWells(TypeSet<MultiID>&);
    void			selectHorizon(MultiID&);
    void			selectFault(MultiID&);
    void			selectPolygonSurface(MultiID&);
    void			selectStickSet(MultiID&);
    bool			selectAttrib(const VisID&,int attrib);

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
    inline uiODViewer2DMgr&	viewer2DMgr()	{ return appl_.viewer2DMgr(); }
    bool			getNewData(const VisID&,int attrib);
    bool			evaluateAttribute(const VisID&,int attrib);
    bool			evaluate2DAttribute(const VisID&,int attrib);
    void			pageUpDownPressed(bool up);
    void			resetServers();
    void			updateColorTable(const VisID&,int attrib);
    void			saveDefColTab(const VisID&,int attrib);
    bool			getDefaultDescID(Attrib::DescID&,
						 bool is2d=false) const;
    void			calcShiftAttribute(int attrib,
						   const Attrib::SelSpec&);
    bool			calcRandomPosAttrib(const VisID&,int attrib);
    bool			calcMultipleAttribs(Attrib::SelSpec&);
    NotifierAccess*		colorTableSeqChange();
    void			addVisDPSChild(CallBacker*);
    void			manSurvCB(CallBacker*);
    void			editSurvCB(CallBacker*);
    void			seisOut2DCB(CallBacker*);
    void			seisOut3DCB(CallBacker*);
    void			editAttr2DCB(CallBacker*)
				    { editAttribSet(true); }
    void			editAttr3DCB(CallBacker*)
				    { editAttribSet(false);}
    Notifier<uiODApplMgr>	attribSetChg;

    void			doVolProcCB(CallBacker*);
    void			doVolProc2DCB(CallBacker*);
    void			doVolProc(const MultiID&,bool is2d);
    void			tieWellToSeismic(CallBacker*);
    void			doWellLogTools(CallBacker*);
    void			launchRockPhysics(CallBacker*);
    void			doLayerModeling(CallBacker*);
    void			setupRdmLinePreview(const TypeSet<Coord>&);
    void			cleanPreview();
    void			addMPEParentPath(const VisID&,const TrcKey&);

    void			enableMenusAndToolBars(bool);
    void			enableTree(bool);
    void			enableSceneManipulation(bool);
				/*!<Turns on/off viewMode and enables/disables
				    the possibility to go to actMode. */
    void			updateCaption() { appl_.updateCaption(); }

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
    VisID			otherFormatVisID() const
				    { return otherformatvisid_; }
    int				otherFormatAttrib() const
				    { return otherformatattrib_; }
    void			useDefColTab(const VisID&,int attrib);
    bool			isRestoringSession() const;
    static void			showReleaseNotes(bool isonline);

protected:
				uiODApplMgr(uiODMain&);
				~uiODApplMgr();

    uiODMain&			appl_;

    uiPickPartServer*		pickserv_;
    uiVisPartServer*		visserv_;
    uiNLAPartServer*		nlaserv_	= nullptr;
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
				MiscSurvInfo()		{}
				~MiscSurvInfo()		{}

	void			refresh();

	OD::XYType		xyunit_			= OD::XYType::Meter;
	SurveyInfo::Unit	zunit_			= SurveyInfo::Second;
	float			zstep_			= 0.004f;
    };

    MiscSurvInfo		tmpprevsurvinfo_;
    static int			manSurv(uiParent*);
    bool			survChgReqAttrUpdate();

    bool			handleEvent(const uiApplPartServer*,
					    int evid) override;
    void*			deliverObject(const uiApplPartServer*,
					      int evid) override;

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

    void			colSeqChg(CallBacker*);
    void			colMapperChg(CallBacker*);
    void			setHistogram(const VisID&,int attrib);
    void			storeEMObject();

    void			manStrat();

    bool			setRandomPosData(const VisID&,int attrib,
						const DataPointSet&);
    void			process2D3D(int opt); /*!<opt=0: create 2D grid,
							  1:3D->2D, 2:2D->3D */

    inline uiODSceneMgr&	sceneMgr()	{ return appl_.sceneMgr(); }

    VisID			otherformatvisid_;
    int				otherformatattrib_	= -1;

    uiVisDataPointSetDisplayMgr* visdpsdispmgr_		= nullptr;

    friend class		uiODMain;

private:

    void			prepSurveyChange() override;
    void			survToBeChanged() override;
    void			survChanged() override;

public:
    void			launch2DViewer(CallBacker*);
};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "integerid.h"
#include "uiapplserv.h"
#include "uistring.h"

class DataPointSet;
class IOObj;
class FilePath;
class uiConvertPos;
class uiDataPointSet;
class uiDataPointSetMan;
class uiDialog;
class uiExp2DGeom;
class uiImp2DGeom;
class uiImpPVDS;
class uiImpRokDocPDF;
class uiExpRokDocPDF;
class uiManPROPS;
class uiODApplMgr;
class uiProbDenFuncMan;
class uiRandomLineMan;
class uiSessionMan;
class uiVelSel;
class ui2DGeomManageDlg;

namespace Attrib { class SelSpec; }
namespace PreStack
{ class uiExportMute; class uiImportMute; class uiBatchProcSetup; }
namespace Vel { class uiImportVelFunc; }


/*!\brief uiApplService for OD */

mExpClass(uiODMain) uiODApplService : public uiApplService
{ mODTextTranslationClass(uiODApplService);
public:
			uiODApplService( uiParent* p, uiODApplMgr& am )
			    : par_(p), applman_(am)	{}
    uiParent*		parent() const;
    bool		eventOccurred(const uiApplPartServer*,int);
    void*		getObject(const uiApplPartServer*, int);

    uiODApplMgr&	applman_;
    uiParent*		par_;
};


/*!\brief Dispatches work for Appl Mgr */

mExpClass(uiODMain) uiODApplMgrDispatcher : public CallBacker
{ mODTextTranslationClass(uiODApplMgrDispatcher);
    friend class	uiODApplMgr;

			uiODApplMgrDispatcher(uiODApplMgr&,uiParent*);
			~uiODApplMgrDispatcher();

    void		survChg(bool);

    void		doOperation(int,int,int);
    void		manPreLoad(int);
    void		posConversion();

    void		processPreStack(bool is2d);
    void		process2D3D(int opt);
    void		genAngleMuteFunction();
    void		bayesClass(bool is2d);
    void		startBatchJob();
    void		setupBatchHosts();
    void		batchProgs();
    void		pluginMan();
    void		manageShortcuts();
    void		startInstMgr();
    void		setAutoUpdatePol();
    void		openXPlot();
    void		createCubeFromWells();
    void		deleteDlgs();

    void		posDlgClose(CallBacker*);
    void		showReleaseNotesCB(CallBacker*);

    uiODApplMgr&	am_;
    uiParent*		par_;
    ObjectSet<uiDataPointSet> uidpsset_;

    uiConvertPos*	convposdlg_ = nullptr;
    uiDataPointSetMan*	mandpsdlg_ = nullptr;
    uiManPROPS*		manpropsdlg_ = nullptr;
    ui2DGeomManageDlg*	man2dgeomdlg_ = nullptr;
    uiProbDenFuncMan*	manpdfdlg_ = nullptr;
    uiSessionMan*	mansessiondlg_ = nullptr;
    uiRandomLineMan*	manrldlg_ = nullptr;
    uiImpPVDS*		impcrossplotdlg_ = nullptr;
    uiExp2DGeom*	exp2dgeomdlg_ = nullptr;
    uiImp2DGeom*	imp2dgeomdlg_ = nullptr;
    uiImpRokDocPDF*	imppdfdlg_ = nullptr;
    uiExpRokDocPDF*	exppdfdlg_ = nullptr;
    PreStack::uiImportMute*	impmutedlg_ = nullptr;
    PreStack::uiExportMute*	expmutedlg_ = nullptr;
    Vel::uiImportVelFunc*	impvelfunc_ = nullptr;
    PreStack::uiBatchProcSetup* batchprocps2ddlg_ = nullptr;
    PreStack::uiBatchProcSetup* batchprocps3ddlg_ = nullptr;
};


/*!\brief Does visualization-related work for uiODApplMgr */

mExpClass(uiODMain) uiODApplMgrAttrVisHandler : public CallBacker
{ mODTextTranslationClass(uiODApplMgrAttrVisHandler);
    friend class	uiODApplMgr;

			uiODApplMgrAttrVisHandler( uiODApplMgr& a, uiParent* p )
			    : am_(a), par_(p)		{}

    void		survChg(bool);
    void		saveNLA(CallBacker*);
    bool		editNLA(bool);
    bool		uvqNLA(bool);
    void		createHorOutput(int tp,bool);
    void		createVol(bool is2d,bool multiattrib);
    void		doXPlot();
    void		crossPlot();
    void		setZStretch();
    bool		selectAttrib(VisID,int);
    void		setHistogram(VisID,int);
    void		colMapperChg();
    void		setRandomPosData(VisID visid,int attrib,
					const DataPointSet&);
    void		pageUpDownPressed(bool);
    void		updateColorTable(VisID,int);
    void		colSeqChg();
    NotifierAccess*	colorTableSeqChange();
    void		useDefColTab(VisID,int);
    void		saveDefColTab(VisID,int);

    uiODApplMgr&	am_;
    uiParent*		par_;
};

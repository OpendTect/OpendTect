#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uiapplserv.h"
#include "uistring.h"

class DataPointSet;
class FilePath;
class IOObj;

class ui2DGeomManageDlg;
class uiConvertPos;
class uiDataPointSet;
class uiDataPointSetMan;
class uiDialog;
class uiExp2DGeom;
class uiExpRokDocPDF;
class uiExportDataPointSet;
class uiImp2DGeom;
class uiImpPVDS;
class uiImpRokDocPDF;
class uiManPROPS;
class uiODApplMgr;
class uiProbDenFuncMan;
class uiRandomLineMan;
class uiSessionMan;
class uiVelSel;

namespace Attrib { class SelSpec; }
namespace PreStack
{ class uiExportMute; class uiImportMute; class uiBatchProcSetup; }
namespace Vel { class uiImportVelFunc; }


/*!\brief uiApplService for OD */

mExpClass(uiODMain) uiODApplService : public uiApplService
{ mODTextTranslationClass(uiODApplService);
public:
			uiODApplService(uiParent*,uiODApplMgr&);
			~uiODApplService();

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

    uiODApplMgr&		am_;
    uiParent*			par_;
    ObjectSet<uiDataPointSet>	uidpsset_;

    uiConvertPos*		convposdlg_			= nullptr;
    uiDataPointSetMan*		mandpsdlg_			= nullptr;
    uiManPROPS*			manpropsdlg_			= nullptr;
    ui2DGeomManageDlg*		man2dgeomdlg_			= nullptr;
    uiProbDenFuncMan*		manpdfdlg_			= nullptr;
    uiSessionMan*		mansessiondlg_			= nullptr;
    uiRandomLineMan*		manrldlg_			= nullptr;
    uiImpPVDS*			impcrossplotdlg_		= nullptr;
    uiExportDataPointSet*	expcrossplotdlg_		= nullptr;
    uiExp2DGeom*		exp2dgeomdlg_			= nullptr;
    uiImp2DGeom*		imp2dgeomdlg_			= nullptr;
    uiImpRokDocPDF*		imppdfdlg_			= nullptr;
    uiExpRokDocPDF*		exppdfdlg_			= nullptr;
    PreStack::uiImportMute*	impmutedlg_			= nullptr;
    PreStack::uiExportMute*	expmutedlg_			= nullptr;
    Vel::uiImportVelFunc*	impvelfunc2d_			= nullptr;
    Vel::uiImportVelFunc*	impvelfunc3d_			= nullptr;
    PreStack::uiBatchProcSetup* batchprocps2ddlg_		= nullptr;
    PreStack::uiBatchProcSetup* batchprocps3ddlg_		= nullptr;
};

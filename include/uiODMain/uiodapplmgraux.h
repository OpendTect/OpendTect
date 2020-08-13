#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiapplserv.h"
#include "uistring.h"

class DataPointSet;
class IOObj;
class uiConvertPos;
class uiDataPointSet;
class uiDataPointSetMan;
class uiDialog;
class uiExp2DGeom;
class uiImp2DGeom;
class uiImpPVDS;
class uiImpRokDocPDF;
class uiExpRokDocPDF;
class uiColSeqImport;
class uiODApplMgr;
class uiProbDenFuncMan;
class uiRandomLineMan;
class uiSessionMan;
class uiVelSel;
class ui2DGeomManageDlg;

namespace Attrib { class SelSpec; }
namespace Coords { class uiConvertGeographicPos; }
namespace File { class Path; }
namespace Vel { class uiImportVelFunc; }
namespace PreStack
{ class uiExportMute; class uiImportMute; class uiBatchProcSetup; }


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
    void		crsPosConversion();

    void		processPreStack(bool is2d);
    void		process2D3D(int opt);
    void		genAngleMuteFunction();
    void		bayesClass(bool is2d);
    void		startBatchJob();
    void		setupBatchHosts();
    void		batchProgs();
    void		pluginMan();
    void		startInstMgr();
    void		setAutoUpdatePol();
    void		openXPlot();
    void		createCubeFromWells();
    void		deleteDlgs();
    void		posDlgClose(CallBacker*);

    uiConvertPos*		convposdlg_;
    uiDataPointSetMan*		mandpsdlg_;
    ui2DGeomManageDlg*		man2dgeomdlg_;
    uiProbDenFuncMan*		manpdfdlg_;
    uiSessionMan*		mansessiondlg_;
    uiRandomLineMan*		manrldlg_;
    uiImpPVDS*			impcrossplotdlg_;
    uiExp2DGeom*		exp2dgeomdlg_;
    uiImp2DGeom*		imp2dgeomdlg_;
    uiImpRokDocPDF*		imppdfdlg_;
    uiExpRokDocPDF*		exppdfdlg_;
    uiColSeqImport*		impcolseqdlg_;
    PreStack::uiImportMute*	impmutedlg_;
    PreStack::uiExportMute*	expmutedlg_;
    Vel::uiImportVelFunc*	impvelfunc_;
    uiODApplMgr&		am_;
    uiParent*			par_;
    ObjectSet<uiDataPointSet>	uidpsset_;
    PreStack::uiBatchProcSetup* batchprocps2ddlg_;
    PreStack::uiBatchProcSetup* batchprocps3ddlg_;
    Coords::uiConvertGeographicPos* convgeoposdlg_;
};


/*!\brief Does visualization-related work for uiODApplMgr */

mExpClass(uiODMain) uiODApplMgrAttrVisHandler : public CallBacker
{ mODTextTranslationClass(uiODApplMgrAttrVisHandler);
    friend class	uiODApplMgr;

			uiODApplMgrAttrVisHandler( uiODApplMgr& a, uiParent* p )
			    : am_(a), par_(p)		{}
    void		survChg(bool);

    bool		editNLA(bool);
    bool		uvqNLA(bool);
    void		createHorOutput(int,bool);
    void		createVol(bool is2d,bool multiattrib);
    void		doXPlot();
    void		crossPlot();
    void		setZStretch();
    bool		selectAttrib(int,int);
    void		setRandomPosData(int visid,int attrib,
					const DataPointSet&);
    void		pageUpDownPressed(bool);
    void		hideColorTable();
    void		updateColorTable(int,int);
    void		colSeqChg();
    void		useDefColTab(int,int);
    void		saveDefColTab(int,int);

    uiODApplMgr&	am_;
    uiParent*		par_;
};

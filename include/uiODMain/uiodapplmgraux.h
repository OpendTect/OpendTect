#ifndef uiodapplmgraux_h
#define uiodapplmgraux_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Mar 2009
 RCS:           $Id: uiodapplmgraux.h,v 1.21 2012/05/03 13:07:42 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class CtxtIOObj;
class DataPointSet;
class uiConvertPos;
class uiDataPointSet;
class uiDialog;
class uiODApplMgr;
class uiSurveyMap;
class uiVelSel;


/*!\brief uiApplService for OD */

mClass uiODApplService : public uiApplService
{
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

mClass uiODApplMgrDispatcher : public CallBacker
{
    friend class	uiODApplMgr;

    			uiODApplMgrDispatcher( uiODApplMgr& a, uiParent* p )
			    : am_(a), par_(p), convposdlg_(0)
			    , basemap_(0), basemapdlg_(0)
			    {}

    void		survChg(bool);

    void		doOperation(int,int,int);
    void		manPreLoad(int);
    void		posConversion();
    int			createMapDataPack(const DataPointSet&,int);

    void		processPreStack();
    void		genAngleMuteFunction();
    void		bayesClass(bool is2d);
    void		resortSEGY();
    void		reStartProc();
    void		batchProgs();
    void		pluginMan();
    void		manageShortcuts();
    void		startInstMgr();
    void		setAutoUpdatePol();
    void		setFonts();
    void		openXPlot();

    void		showBaseMap();
    uiSurveyMap*	basemap_;
    uiDialog*		basemapdlg_;

    void		posDlgClose(CallBacker*);
    uiConvertPos*	convposdlg_;
    uiODApplMgr&	am_;
    uiParent*		par_;
    ObjectSet<uiDataPointSet> uidpsset_;

    void		process2D3D(bool to2d);
    void		createCubeFromWells();
};


/*!\brief Does visualisation-related work for uiODApplMgr */

mClass uiODApplMgrAttrVisHandler : public CallBacker
{
    friend class	uiODApplMgr;

    			uiODApplMgrAttrVisHandler( uiODApplMgr& a, uiParent* p )
			    : am_(a), par_(p)		{}
    void		survChg(bool);

    bool		editNLA(bool);
    void		createHorOutput(int,bool);
    void		createVol(bool);
    void		doXPlot();
    void		crossPlot();
    void		setZStretch();
    bool		selectAttrib(int,int);
    void		setHistogram(int,int);
    void		colMapperChg();
    void		createAndSetMapDataPack(int,int,
	    					const DataPointSet&,int);
    void		pageUpDownPressed(bool);
    void		updateColorTable(int,int);
    void		colSeqChg();
    NotifierAccess*	colorTableSeqChange();
    void		useDefColTab(int,int);
    void		saveDefColTab(int,int);

    uiODApplMgr&	am_;
    uiParent*		par_;
};


#endif

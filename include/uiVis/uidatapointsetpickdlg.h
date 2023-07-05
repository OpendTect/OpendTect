#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

#include "datapointset.h"
#include "emposid.h"

class uiTable;
class uiToolBar;
class Array2DInterpol;

namespace Pick { class SetMgr; }
namespace visSurvey { class PickSetDisplay; }

mExpClass(uiVis) uiDataPointSetPickDlg : public uiDialog
{
mODTextTranslationClass(uiDataPointSetPickDlg)
public:
    virtual		~uiDataPointSetPickDlg();

protected:
			uiDataPointSetPickDlg(uiParent*,const SceneID&);

    void		initPickSet();
    void		updateDPS();
    void		updateTable();
    void		updateButtons();
    virtual void	cleanUp();

    void		valChgCB(CallBacker*);
    void		rowClickCB(CallBacker*);
    void		pickModeCB(CallBacker*);
    void		openCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		saveasCB(CallBacker*);
    void		doSave(bool saveas);
    void		pickCB(CallBacker*);
    void		locChgCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		winCloseCB(CallBacker*);
    void		objSelCB(CallBacker*);

    VisID		sceneid_;
    uiTable*		table_;
    uiToolBar*		tb_;
    RefMan<DataPointSet> dps_;
    TypeSet<float>	values_;
    visSurvey::PickSetDisplay* psd_				= nullptr;
    Pick::SetMgr&	picksetmgr_;
    int			pickbutid_;
    int			savebutid_;
    bool		changed_				= false;
};


mExpClass(uiVis) uiEMDataPointSetPickDlg : public uiDataPointSetPickDlg
{
mODTextTranslationClass(uiEMDataPointSetPickDlg)
public:
			uiEMDataPointSetPickDlg(uiParent*,const SceneID&,
						const EM::ObjectID&);
			~uiEMDataPointSetPickDlg();

    const DataPointSet*	getData() const		{ return emdps_; }
    Notifier<uiEMDataPointSetPickDlg> readyForDisplay;

protected:

    RefMan<DataPointSet> emdps_;
    EM::ObjectID	emid_;
    Array2DInterpol*	interpol_				= nullptr;

    int			addSurfaceData();
    int			dataidx_				= -1;

    virtual void	cleanUp();
    void		interpolateCB(CallBacker*);
    void		settCB(CallBacker*);

    TrcKeySampling		tks_;
};

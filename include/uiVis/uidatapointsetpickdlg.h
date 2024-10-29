#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"

#include "datapointset.h"
#include "emposid.h"
#include "uidialog.h"
#include "vispicksetdisplay.h"

class uiTable;
class uiToolBar;
class Array2DInterpol;
class uiVisPartServer;

namespace Pick { class SetMgr; }

mExpClass(uiVis) uiDataPointSetPickDlg : public uiDialog
{
mODTextTranslationClass(uiDataPointSetPickDlg)
public:
			~uiDataPointSetPickDlg();

protected:
			uiDataPointSetPickDlg(uiParent*,uiVisPartServer*,
					      const SceneID&);

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
    bool		acceptOK(CallBacker*) override;
    void		winCloseCB(CallBacker*);
    void		objSelCB(CallBacker*);

    uiVisPartServer*	vispartserv_;
    SceneID		sceneid_;
    uiTable*		table_;
    uiToolBar*		tb_;
    RefMan<DataPointSet> dps_;
    TypeSet<float>	values_;
    RefMan<visSurvey::PickSetDisplay> psd_;
    Pick::SetMgr&	picksetmgr_;
    int			pickbutid_;
    int			savebutid_;
    bool		changed_	= false;
};


mExpClass(uiVis) uiEMDataPointSetPickDlg : public uiDataPointSetPickDlg
{
mODTextTranslationClass(uiEMDataPointSetPickDlg)
public:
			uiEMDataPointSetPickDlg(uiParent*,uiVisPartServer*,
						const SceneID&,
						const EM::ObjectID&);
			~uiEMDataPointSetPickDlg();

    const DataPointSet* getData() const		{ return emdps_.ptr(); }
    Notifier<uiEMDataPointSetPickDlg> readyForDisplay;

protected:

    RefMan<DataPointSet> emdps_;
    EM::ObjectID	emid_;
    Array2DInterpol*	interpol_				= nullptr;

    int			addSurfaceData();
    int			dataidx_				= -1;

    void		cleanUp() override;
    void		interpolateCB(CallBacker*);
    void		settCB(CallBacker*);

    TrcKeySampling	tks_;
};

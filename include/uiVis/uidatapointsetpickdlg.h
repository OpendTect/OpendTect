#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2006
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "emobject.h"
#include "sets.h"

class uiTable;
class uiToolBar;
class Array2DInterpol;
class DataPointSet;

namespace Pick { class Set; }
namespace visSurvey { class PickSetDisplay; }

mExpClass(uiVis) uiDataPointSetPickDlg : public uiDialog
{ mODTextTranslationClass(uiDataPointSetPickDlg);
public:
			uiDataPointSetPickDlg(uiParent*,int sceneid);
    virtual		~uiDataPointSetPickDlg();

    Pick::Set*		pickSet();

protected:

    void		initPickSet();
    void		updateDPS();
    void		updateTable();
    void		updateButtons();
    void		rebuildFromPS();
    virtual void	cleanUp();

    void		valChgCB(CallBacker*);
    void		rowClickCB(CallBacker*);
    void		pickModeCB(CallBacker*);
    void		openCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		saveasCB(CallBacker*);
    void		doSave(bool saveas);
    void		setChgCB(CallBacker*);
    bool		acceptOK();
    void		winCloseCB(CallBacker*);
    void		objSelCB(CallBacker*);

    int			sceneid_;
    uiTable*		table_;
    uiToolBar*		tb_;
    DataPointSet&	dps_;
    TypeSet<float>	values_;
    visSurvey::PickSetDisplay* psd_;
    int			pickbutid_;
    int			savebutid_;
    bool		changed_;
};


mExpClass(uiVis) uiEMDataPointSetPickDlg : public uiDataPointSetPickDlg
{ mODTextTranslationClass(uiEMDataPointSetPickDlg);
public:
			uiEMDataPointSetPickDlg(uiParent*,int sceneid,
						const DBKey&);
			~uiEMDataPointSetPickDlg();

    const DataPointSet&	getData() const		{ return emdps_; }
    Notifier<uiEMDataPointSetPickDlg> readyForDisplay;

protected:

    DataPointSet&	emdps_;
    DBKey		emid_;
    Array2DInterpol*	interpol_;

    int			addSurfaceData();
    int			dataidx_;

    virtual void	cleanUp();
    void		interpolateCB(CallBacker*);
    void		settCB(CallBacker*);

    TrcKeySampling	tks_;
};

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "ptrman.h"

class uiTabStack;
class uiWellDispProperties;
class uiLabeledComboBox;

namespace Well { class Data; class DisplayProperties; };

/*!
\brief Well display properties dialog box.
*/

mExpClass(uiWell) uiWellDispPropDlg : public uiDialog
{mODTextTranslationClass(uiWellDispPropDlg)
public:
				uiWellDispPropDlg(uiParent*,const MultiID&,
						  bool is2ddisplay,
						  OD::Color bgcolor);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg> saveReq;
    Notifier<uiWellDispPropDlg> applyTabReq;
    Notifier<uiWellDispPropDlg> resetAllReq;

    enum TabType { LeftLog=0, CenterLog=1, RightLog=2, Marker=3, Track=4 };

    virtual Well::Data*		wellData()		{ return wd_; }
    virtual const Well::Data*	wellData() const	{ return wd_; }

    TabType			currentTab() const;
    bool			is2D() const		{ return is2ddisplay_; }
    bool			needsSave() const;

    void			setNeedsSave(bool yn);

protected:
				uiWellDispPropDlg(uiParent*,Well::Data*,
						  bool is2ddisplay,
						  OD::Color bgcolor);

    RefMan<Well::Data>		wd_;
    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;
    OD::Color			bkcol_;

    virtual void		getFromScreen();
    virtual void		putToScreen();

    void			initDlg(OD::Color);
    void			postFinalizeCB(CallBacker*);
    virtual void		setWDNotifiers(bool yn);

    virtual void		applyTabPush(CallBacker*);
    virtual void		resetAllPush(CallBacker*);
    void			saveAsDefaultCB(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    virtual void		resetCB(CallBacker*);

    virtual void		propChg(CallBacker*);
    void			markersChgd(CallBacker*);
    void			logsChgd(CallBacker*);
    void			wdChg(CallBacker*);
    void			welldataDelNotify(CallBacker*);
    void			tabSel(CallBacker*);
};


/*!
\brief Multi Well display properties dialog box.
*/

mExpClass(uiWell) uiMultiWellDispPropDlg : public uiWellDispPropDlg
{mODTextTranslationClass(uiMultiWellDispPropDlg)
public:
				uiMultiWellDispPropDlg(uiParent*,
						const ObjectSet<Well::Data>&,
						bool is2ddisplay,
						OD::Color bgcolor);
				~uiMultiWellDispPropDlg();

protected:

    ObjectSet<Well::Data>	wds_;
    uiLabeledComboBox*		wellselfld_ = nullptr;
    bool			allapplied_ = false;

    void			setWDNotifiers(bool yn) override;

    void			applyTabPush(CallBacker*) override;
    void			resetAllPush(CallBacker*) override;
    bool			acceptOK(CallBacker*) override;

    void			resetProps(int,int);
    virtual void		wellSelChg(CallBacker*);
    void			saveWellDispProps(const Well::Data*);
    void			saveAllWellDispProps();
    bool			dispPropsChanged(int) const;
    bool			dispPropsChanged() const;


};


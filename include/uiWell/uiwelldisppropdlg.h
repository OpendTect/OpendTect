#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
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

    Well::Data*			wellData()		{ return wd_; }
    const Well::Data*		wellData() const	{ return wd_; }

    TabType			currentTab() const;
    bool			is2D() const		{ return is2ddisplay_; }
    void			updateLogs();

protected:

    RefMan<Well::Data>		wd_;
    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;
    OD::Color			bkcol_;

    virtual void		getFromScreen();
    virtual void		putToScreen();

    virtual void		setWDNotifiers(bool yn);
    bool			acceptOK(CallBacker*) override;
    virtual void		resetCB(CallBacker*);
    virtual void		applyTabPush(CallBacker*);
    virtual void		resetAllPush(CallBacker*);
    virtual void		onClose(CallBacker*)		{}

    virtual void		propChg(CallBacker*);
    void			markersChgd(CallBacker*);
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

    void			resetProps(int,int);
    virtual void 		wellSelChg(CallBacker*);
    void			setWDNotifiers(bool yn) override;
    void			onClose(CallBacker*) override;
    void			applyTabPush(CallBacker*) override;
    void			resetAllPush(CallBacker*) override;
};


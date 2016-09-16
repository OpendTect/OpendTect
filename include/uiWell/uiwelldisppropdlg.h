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

class uiTabStack;
class uiWellDispProperties;
class uiLabeledComboBox;

namespace Well { class Data; };

/*!
\brief Well display properties dialog box.
*/

mExpClass(uiWell) uiWellDispPropDlg : public uiDialog
{mODTextTranslationClass(uiWellDispPropDlg)
public:
				uiWellDispPropDlg(uiParent*,Well::Data*,
						  bool is2ddisplay=false);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg>	applyAllReq;

    Well::Data*			wellData()		{ return wd_.ptr(); }
    const Well::Data*		wellData() const	{ return wd_.ptr(); }


    bool 			savedefault_;
    void			updateLogs();
 
protected:

    RefMan<Well::Data>		wd_;
    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;

    virtual void		getFromScreen();
    virtual void		putToScreen();

    virtual void		setWDNotifiers(bool yn);

    virtual void		applyAllPush(CallBacker*);
    virtual void		onClose(CallBacker*);
    virtual void		propChg(CallBacker*);
    bool			rejectOK();
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
						bool is2ddisplay);
				~uiMultiWellDispPropDlg();

protected:

    ObjectSet<Well::Data> 	wds_;
    uiLabeledComboBox*		wellselfld_;

    void			resetProps(int logidx);
    virtual void 		wellSelChg(CallBacker*);
    virtual void		setWDNotifiers(bool yn);
    void			onClose(CallBacker*);
};

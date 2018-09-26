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
#include "uitabstack.h"

class uiTabStack;
class uiWellDispProperties;
class uiLabeledComboBox;
class uiPanelTab;
class uiWellLogDispProperties;

namespace Well { class Data; };


class uiPanelTab : public uiTabStack
{mODTextTranslationClass(uiPanelTab)
public:
		uiPanelTab(uiParent*,Well::Data& welldata,
			   const char* panelnm,const bool is2ddisp);
		~uiPanelTab();

    Well::Data&		welldata_;
    const bool		is2ddisp_;

    void		logTabSelChgngeCB(CallBacker*);
    void		lognmChg(CallBacker*);
    void		logTabClosedCB(CallBacker*);
    void		logTabToBeClosedCB(CallBacker*);

    uiGroup*		createLogPropertiesGrp();
    void		addLogPanel();
    void		init();
    void		showLogTabCloseButton();
};

/*!
\brief Well display properties dialog box.
*/

mExpClass(uiWell) uiWellDispPropDlg : public uiDialog
{mODTextTranslationClass(uiWellDispPropDlg)
public:
				uiWellDispPropDlg(uiParent*,Well::Data*,
						  bool is2ddisplay=false,
						  bool multipanel=false);
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
    bool			multipanel_;

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
    void			logTabSelChgngeCB(CallBacker*);
    void			tabRemovedCB(CallBacker*);

    void			createMultiPanelUI();
    void			createSinglePanelUI();

    void			addPanel();
    void			addMarkersPanel();
    void			updatePanelNames();
    void			showPanelTabCloseButton();
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

    ObjectSet<Well::Data>	wds_;
    uiLabeledComboBox*		wellselfld_;

    void			resetProps(int logidx);
    virtual void 		wellSelChg(CallBacker*);
    virtual void		setWDNotifiers(bool yn);
    void			onClose(CallBacker*);
};

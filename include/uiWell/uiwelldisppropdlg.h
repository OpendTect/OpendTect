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
#include "uigroup.h"
#include "uitabstack.h"
#include "welldisp.h"

class uiLabeledComboBox;
class uiMultiWellDispPropGrp;
class uiPanelTab;
class uiTabStack;
class uiWellDispPropGrp;
class uiWellDispProperties;

namespace Well { class Data; class DisplayProperties2D; };


class uiPanelTab : public uiTabStack
{mODTextTranslationClass(uiPanelTab)
public:
		uiPanelTab(uiParent*,Well::Data& welldata,
			   Well::DisplayProperties2D::LogPanelProps&,
			   const char* panelnm,const bool is2ddisp);
		~uiPanelTab();

protected:
    void		init();
    void		addLog();
    void		addLogToPanel();
    void		removeLogFromPanel(int);
    uiGroup*		createLogPropertiesGrp();

    Well::DisplayProperties2D::LogPanelProps& logpanel_;

private:

    void		logTabSelChgngeCB(CallBacker*);
    void		logTabClosedCB(CallBacker*);
    void		logTabToBeClosedCB(CallBacker*);
    void		logpropChg(CallBacker*);
    void		logPanelChgCB(CallBacker*);

    void		showLogTabCloseButton();

    Well::Data&		welldata_;
    const bool		is2ddisp_;
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

    bool			rejectOK();
    uiWellDispPropGrp*	welldisppropgrp_;
    bool			savedefault_;
};


mExpClass(uiWell) uiWellDispPropGrp : public uiGroup
{mODTextTranslationClass(uiWellDispPropGrp)
public:
				uiWellDispPropGrp(uiParent*,Well::Data*,
						  bool is2ddisplay=false,
						  bool multipanel=false);
				~uiWellDispPropGrp();

    Notifier<uiWellDispPropGrp> applyAllReq;

    RefMan<Well::Data>			wellData()
					{ return wd_.ptr(); }
    ConstRefMan<Well::Data>	wellData() const
					{ return wd_.ptr(); }
    void			updateLogs();

    friend class uiMultiWellDispPropGrp;
protected:

    RefMan<Well::Data>		wd_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;
    bool			multipanel_;
    uiTabStack*			ts_;

    void			getFromScreen();
    void			putToScreen();

    void			applyAllPush(CallBacker*);
    virtual void		propChg(CallBacker*);
    void			welldataDelNotify(CallBacker*);
    void			tabSel(CallBacker*);
    void			tabRemovedCB(CallBacker*);
    void			markerpropChg(CallBacker*);

    void			createMultiPanelUI();
    void			createSinglePanelUI();

    void			addLogPanel();
    void			addPanelTab();
    void			addMarkersPanel();
    void			updatePanelNames();
    void			showPanelTabCloseButton();
    void			onClose(CallBacker*);
    void			propChgCB(CallBacker*);
};


/*!
\brief Multi Well display properties dialog box.
*/

mExpClass(uiWell) uiMultiWellDispPropGrp : public uiGroup
{mODTextTranslationClass(uiMultiWellDispPropGrp)
public:
				uiMultiWellDispPropGrp(uiParent*,
						const ObjectSet<Well::Data>&,
						bool is2ddisplay);
				~uiMultiWellDispPropGrp();
    int				curWellID();
    uiWellDispPropGrp*		curWellDispPropGrp();
    void			resetProps(int logidx);

protected:

    ObjectSet<Well::Data>	wds_;
    uiLabeledComboBox*		wellselfld_;
    ObjectSet<uiWellDispPropGrp> welldisppropgrps_;

    void			wellSelChg(CallBacker*);
};

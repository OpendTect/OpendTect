#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2015
 RCS:		$Id:$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uidialog.h"

class uiListBox;
class uiCheckBox;
class uiGenInput;
class uiTextEdit;
namespace Repos { class IOParSet; class IOPar; }


mExpClass(uiSEGYTools) uiSEGYImpParsDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYImpParsDlg)
public:

			uiSEGYImpParsDlg(uiParent*,bool,const char*);
			~uiSEGYImpParsDlg();

    const char*		parName() const		{ return parname_; }

protected:

    uiListBox*		listfld_;
    uiButton*		renbut_;
    uiButton*		delbut_;
    Repos::IOParSet&	parset_;

    mutable BufferString parname_;
    mutable bool	setchgd_;

    int			parIdx() const;
    void		fillList();
    void		update(const char*);
    void		updateButtons();

    void		renCB(CallBacker*);
    void		delCB(CallBacker*);
    void		selChgCB(CallBacker*);
    bool		acceptOK();

    virtual void	selectionChanged()	{}
    virtual bool	doIO()			= 0;

};


mExpClass(uiSEGYTools) uiSEGYReadImpParsDlg : public uiSEGYImpParsDlg
{ mODTextTranslationClass(uiSEGYReadImpParsDlg)
public:

			uiSEGYReadImpParsDlg(uiParent*,const char* defnm=0);

    const IOPar*	pars() const;

protected:

    virtual bool	doIO();
    uiTextEdit*		detailsfld_;

    virtual void	selectionChanged();

};


mExpClass(uiSEGYTools) uiSEGYStoreImpParsDlg : public uiSEGYImpParsDlg
{ mODTextTranslationClass(uiSEGYStoreImpParsDlg)
public:

			uiSEGYStoreImpParsDlg(uiParent*,const IOPar&,
						const char* defnm=0,
						bool vintagemode=false);
			~uiSEGYStoreImpParsDlg();

protected:

    uiGenInput*		namefld_;
    uiCheckBox*		asdefbox_;
    Repos::IOPar*	parstostore_;

    virtual bool	doIO();
    virtual void	selectionChanged();

};

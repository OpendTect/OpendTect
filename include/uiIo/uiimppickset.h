#ifndef uiimppickset_h
#define uiimppickset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "multiid.h"

class uiCheckBox;
class uiColorInput;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiLabeledComboBox;
class uiPickPartServer;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for pickset selection */

mExpClass(uiIo) uiImpExpPickSet : public uiDialog
{ mODTextTranslationClass(uiImpExpPickSet);
public:
			uiImpExpPickSet(uiParent*,uiPickPartServer*,bool);
			~uiImpExpPickSet();

    const MultiID&	getStoredID() const	{ return storedid_; }

    Notifier<uiImpExpPickSet> importReady;

protected:

    uiIOObjSel*		objfld_;
    uiIOObjSel*		horinputfld_;
    uiColorInput*	colorfld_;
    uiCheckBox*		polyfld_;
    uiLabeledComboBox*	zfld_;
    uiLabeledComboBox*  horinpfld_;
    uiGenInput*		constzfld_;
    uiTableImpDataSel*  dataselfld_;
    uiFileInput*	filefld_;

    uiPickPartServer*	serv_;

    virtual bool	acceptOK(CallBacker*);
    void		inputChgd(CallBacker*);
    void		formatSel(CallBacker*);
    bool		checkInpFlds();
    bool		doImport();
    bool		doExport();

    bool		import_;
    Table::FormatDesc&  fd_;

    MultiID		storedid_;
};


#endif


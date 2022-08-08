#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "multiid.h"
#include "uicoordsystem.h"

class uiCheckBox;
class uiColorInput;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiLabeledComboBox;
class uiPickPartServer;
class uiTableImpDataSel;
class uiGenInput;
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
    uiGenInput*		polyfld_;
    uiLabeledComboBox*	zfld_;
    uiLabeledComboBox*  horinpfld_;
    uiGenInput*		constzfld_;
    uiTableImpDataSel*  dataselfld_;
    uiFileInput*	filefld_;
    Coords::uiCoordSystemSel* coordsysselfld_;
    uiPickPartServer*	serv_;

    bool		acceptOK(CallBacker*) override;
    void		inputChgd(CallBacker*);
    void		formatSel(CallBacker*);
    bool		checkInpFlds();
    bool		doImport();
    bool		doExport();

    bool		import_;
    Table::FormatDesc&  fd_;

    MultiID		storedid_;

private:
    static uiString	sPicksetPolygon() { return tr("Pointset/Polygon"); }
};



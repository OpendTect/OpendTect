#ifndef uiimppickset_h
#define uiimppickset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.h,v 1.9 2009-01-08 07:23:07 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class IOObj;
class uiCheckBox;
class uiColorInput;
class uiLabeledComboBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiPickPartServer;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for pickset selection */

mClass uiImpExpPickSet : public uiDialog
{
public:
			uiImpExpPickSet(uiPickPartServer*,bool);
			~uiImpExpPickSet();

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
    void		formatSel(CallBacker*);
    bool		checkInpFlds();
    bool		doImport();
    bool		doExport();

    bool		import_;
    CtxtIOObj&		ctio_;
    Table::FormatDesc&  fd_;
};


#endif

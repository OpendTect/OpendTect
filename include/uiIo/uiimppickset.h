#ifndef uiimppickset_h
#define uiimppickset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.h,v 1.12 2012-03-26 18:18:32 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
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
};


#endif

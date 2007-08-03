#ifndef uiimppickset_h
#define uiimppickset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.h,v 1.5 2007-08-03 09:49:13 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class CtxtIOObj;
class uiLabeledComboBox;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }

/*! \brief Dialog for pickset selection */

class uiImpExpPickSet : public uiDialog
{
public:
			uiImpExpPickSet(uiParent*,bool);
			~uiImpExpPickSet();

protected:

    uiIOObjSel*		objfld_;
    uiGenInput*		xyfld_;
    uiLabeledComboBox*	zfld_;
    uiGenInput*		constzfld_;
    uiTableImpDataSel*  dataselfld_;
    uiFileInput*	filefld_;


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

#ifndef uitranslatedlg_h
#define uitranslatedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id: uitranslatedlg.h,v 1.1 2010-10-26 06:41:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiCheckBox;
class uiComboBox;

mClass uiTranslateDlg : public uiDialog
{
public:
			uiTranslateDlg(uiParent*);
			~uiTranslateDlg();

protected:

    void		fillBox();
    bool		acceptOK(CallBacker*);

    uiComboBox*		languagefld_;
    uiCheckBox*		enabbut_;

};

#endif


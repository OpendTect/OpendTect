#ifndef uigoogleexp2dlines_h
#define uigoogleexp2dlines_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
 * ID       : $Id: uigoogleexp2dlines.h,v 1.1 2009-11-11 14:08:16 cvsbert Exp $
-*/

#include "uidialog.h"
#include "multiid.h"
class uiListBox;
class uiSeisSel;
class uiFileInput;
class uiSelLineStyle;


class uiGoogleExport2DSeis : public uiDialog
{
public:

			uiGoogleExport2DSeis(uiParent*);
			~uiGoogleExport2DSeis();

protected:

    uiSeisSel*		inpfld_;
    uiListBox*		selfld_;
    uiSelLineStyle*	lsfld_;
    uiFileInput*	fnmfld_;

    void		initWin(CallBacker*);
    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

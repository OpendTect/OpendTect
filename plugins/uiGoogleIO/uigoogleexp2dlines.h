#ifndef uigoogleexp2dlines_h
#define uigoogleexp2dlines_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
 * ID       : $Id: uigoogleexp2dlines.h,v 1.2 2009-11-11 15:28:27 cvsbert Exp $
-*/

#include "uidialog.h"
#include "multiid.h"
class uiGenInput;
class uiFileInput;
class uiSelLineStyle;
class uiSeis2DFileMan;


class uiGoogleExport2DSeis : public uiDialog
{
public:

			uiGoogleExport2DSeis(uiSeis2DFileMan*);
			~uiGoogleExport2DSeis();

protected:

    uiSeis2DFileMan*	s2dfm_;

    uiGenInput*		putlnmfld_;
    uiSelLineStyle*	lsfld_;
    uiFileInput*	fnmfld_;

    bool		acceptOK(CallBacker*);

};


#endif

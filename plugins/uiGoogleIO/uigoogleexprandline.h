#ifndef uigoogleexprandline_h
#define uigoogleexprandline_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
 * ID       : $Id: uigoogleexprandline.h,v 1.1 2009/11/17 15:28:55 cvsbert Exp $
-*/

#include "uigoogleexpdlg.h"
class Coord;
class uiGenInput;
class uiSelLineStyle;
template <class T> class TypeSet;
namespace ODGoogle { class XMLWriter; }


class uiGoogleExportRandomLine : public uiDialog
{
public:

			uiGoogleExportRandomLine(uiParent*,
					const TypeSet<Coord>&,const char*);

protected:

    const TypeSet<Coord>& crds_;

    uiGenInput*		putlnmfld_;
    uiGenInput*		lnmfld_;
    uiSelLineStyle*	lsfld_;

			mDecluiGoogleExpStd;

    void		putSel(CallBacker*);

};


#endif

#ifndef uigoogleexprandline_h
#define uigoogleexprandline_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
 * ID       : $Id$
-*/

#include "uigoogleexpdlg.h"
class Coord;
class uiGenInput;
class uiSelLineStyle;
template <class T> class TypeSet;
namespace ODGoogle { class XMLWriter; }


mClass(uiGoogleIO) uiGoogleExportRandomLine : public uiDialog
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

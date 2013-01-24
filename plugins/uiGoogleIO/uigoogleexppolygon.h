#ifndef uigoogleexppolygon_h
#define uigoogleexppolygon_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
 * ID       : $Id$
-*/

#include "uigoogleexpdlg.h"
class uiGenInput;
class uiSelLineStyle;
namespace Pick { class Set; }


mClass(uiGoogleIO) uiGoogleExportPolygon : public uiDialog
{
public:

			uiGoogleExportPolygon(uiParent*,const Pick::Set&);

protected:

    const Pick::Set&	ps_;

    uiSelLineStyle*	lsfld_;
    uiGenInput*		hghtfld_;

			mDecluiGoogleExpStd;

};


#endif

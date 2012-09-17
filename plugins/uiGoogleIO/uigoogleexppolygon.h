#ifndef uigoogleexppolygon_h
#define uigoogleexppolygon_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
 * ID       : $Id: uigoogleexppolygon.h,v 1.1 2009/11/16 15:45:52 cvsbert Exp $
-*/

#include "uigoogleexpdlg.h"
class uiGenInput;
class uiSelLineStyle;
namespace Pick { class Set; }


class uiGoogleExportPolygon : public uiDialog
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

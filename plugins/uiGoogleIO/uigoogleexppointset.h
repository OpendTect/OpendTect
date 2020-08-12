#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Wayne Mogg
 * DATE     : Dec 2019
 * ID	    : $Id$
-*/

#include "uigoogleexpdlg.h"
class uiGenInput;
class uiSelLineStyle;
namespace Pick { class Set; }


mClass(uiGoogleIO) uiGoogleExportPointSet : public uiDialog
{ mODTextTranslationClass(uiGoogleExportPointSet);
public:

			uiGoogleExportPointSet(uiParent*,const Pick::Set&);

protected:

    const Pick::Set&	ps_;

    uiGenInput*		hghtfld_;

			mDecluiGoogleExpStd;

};



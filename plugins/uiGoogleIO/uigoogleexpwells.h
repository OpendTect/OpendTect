#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2009
 * ID       : $Id$
-*/

#include "uigoogleexpdlg.h"
#include "multiid.h"
class uiListBox;


mClass(uiGoogleIO) uiGoogleExportWells : public uiDialog
{ mODTextTranslationClass(uiGoogleExportWells);
public:

			uiGoogleExportWells(uiParent*);
			~uiGoogleExportWells();

protected:

    ObjectSet<MultiID>	wellids_;

    uiListBox*		selfld_;

    void		initWin(CallBacker*);


			mDecluiGoogleExpStd;
};



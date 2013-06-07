#ifndef uigoogleexpwells_h
#define uigoogleexpwells_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2009
 * ID       : $Id: uigoogleexpwells.h,v 1.3 2009/11/16 13:56:10 cvsbert Exp $
-*/

#include "uigoogleexpdlg.h"
#include "multiid.h"
class uiListBox;


class uiGoogleExportWells : public uiDialog
{
public:

			uiGoogleExportWells(uiParent*);
			~uiGoogleExportWells();

protected:

    ObjectSet<MultiID>	wellids_;

    uiListBox*		selfld_;

    void		initWin(CallBacker*);


			mDecluiGoogleExpStd;
};


#endif

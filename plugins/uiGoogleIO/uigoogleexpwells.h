#ifndef uigoogleexpwells_h
#define uigoogleexpwells_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2009
 * ID       : $Id: uigoogleexpwells.h,v 1.1 2009-10-12 12:34:33 cvsbert Exp $
-*/

#include "uidialog.h"
#include "multiid.h"
class uiFileInput;
class uiListBox;
namespace Well { class Info; }


class uiGoogleExportWells : public uiDialog
{
public:

			uiGoogleExportWells(uiParent*);
			~uiGoogleExportWells();

protected:

    uiListBox*		selfld_;
    uiFileInput*	fnmfld_;

    ObjectSet<MultiID>	wellids_;

    void		initWin(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		writeWell(std::ostream&,const char*,const Well::Info&);

};


#endif

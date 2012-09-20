#ifndef uitutwelltools_h
#define uitutwelltools_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
 * ID       : $Id$
-*/

#include "uidialog.h"
#include "multiid.h"

class uiGenInput;
class uiLabeledListBox;
class uiLabeledSpinBox;
namespace Tut { class LogTools; }
namespace Well { class Data; }


class uiTutWellTools : public uiDialog
{
public:

    			uiTutWellTools(uiParent*,const MultiID& wellid);
			~uiTutWellTools();

protected:
    uiLabeledListBox*	inplogfld_;
    uiGenInput*		outplogfld_;
    uiLabeledSpinBox*	gatefld_;

    BufferString	inlognm_;
    BufferString        outlognm_;
    Well::Data*		wd_;
    MultiID		wellid_;

    void		inpchg(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

#ifndef uitutwelltools_h
#define uitutwelltools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
 * ID       : $Id: uitutwelltools.h,v 1.2 2009-04-09 11:49:08 cvsranojay Exp $
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

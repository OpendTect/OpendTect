#ifndef uitutwelltools_h
#define uitutwelltools_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R.K. Singh
 * DATE     : June 2007
 * ID       : $Id: uitutwelltools.h,v 1.1 2007-06-19 10:01:43 cvsraman Exp $
-*/

#include "uidialog.h"
#include "viswell.h"
#include "welldata.h"
#include "multiid.h"

class uiGenInput;
class uiLabeledListBox;
class uiLabeledSpinBox;
namespace Tut { class LogTools; }


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

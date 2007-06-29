#ifndef uimadbldcmd_h
#define uimadbldcmd_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: uimadbldcmd.h,v 1.3 2007-06-29 11:58:53 cvsbert Exp $
-*/

#include "uidialog.h"
class uiGenInput;
class uiSeparator;
class uiComboBox;
class uiListBox;


class uiMadagascarBldCmd : public uiDialog
{
public:

			uiMadagascarBldCmd(uiParent*,BufferString&);
			~uiMadagascarBldCmd();

protected:

    uiGenInput*		cmdfld_;
    uiComboBox*		groupfld_;
    uiListBox*		progfld_;
    BufferString&	cmd_;

    uiSeparator*	createMainPart();
    bool		acceptOK(CallBacker*);

};


#endif

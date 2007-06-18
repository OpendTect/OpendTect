#ifndef uimadbldcmd_h
#define uimadbldcmd_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: uimadbldcmd.h,v 1.1 2007-06-18 16:39:49 cvsbert Exp $
-*/

#include "uidialog.h"
class uiGenInput;


class uiMadagascarBldCmd : public uiDialog
{
public:

			uiMadagascarBldCmd(uiParent*,BufferString&);
			~uiMadagascarBldCmd();

protected:

    uiGenInput*		cmdfld_;
    BufferString&	cmd_;

    bool		acceptOK(CallBacker*);

};


#endif

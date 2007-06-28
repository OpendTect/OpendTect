#ifndef uimadbldcmd_h
#define uimadbldcmd_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
 * ID       : $Id: uimadbldcmd.h,v 1.2 2007-06-28 18:11:32 cvsbert Exp $
-*/

#include "uidialog.h"
class uiGenInput;
class uiSeparator;


class uiMadagascarBldCmd : public uiDialog
{
public:

			uiMadagascarBldCmd(uiParent*,BufferString&);
			~uiMadagascarBldCmd();

protected:

    uiGenInput*		cmdfld_;
    BufferString&	cmd_;

    uiSeparator*	createMainPart();
    bool		acceptOK(CallBacker*);

};


#endif

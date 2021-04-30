#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
-*/

#include "uibouncymod.h"
#include "uidialog.h"

class uiGenInput;

namespace uiBouncy
{
class uiBouncySettingsDlg;

mExpClass(uiBouncy) uiBouncyMain : public uiDialog
{
public:

				uiBouncyMain(uiParent*, uiBouncySettingsDlg**); 
				~uiBouncyMain();

    BufferString		getPlayerName() const;
    void			setPlayerName(const BufferString&);

protected:

    uiGenInput*			namefld_;
    uiBouncySettingsDlg**	bsdlg_;

    bool			acceptOK(CallBacker*);

};
};


#ifndef uihorizonmergedlg_h
#define uihorizonmergedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2011
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class uiGenInput;
class uiHorizon3DSel;
class uiSurfaceWrite;

mExpClass(uiEarthModel) uiHorizonMergeDlg : public uiDialog
{ mODTextTranslationClass(uiHorizonMergeDlg);
public:
			uiHorizonMergeDlg(uiParent*,bool is2d);
			~uiHorizonMergeDlg();

    DBKey		getNewHorMid() const;
    void		setInputHors(const TypeSet<DBKey>& mids);

protected:

    bool		acceptOK();

    uiHorizon3DSel*	horselfld_;
    uiGenInput*		duplicatefld_;
    uiSurfaceWrite*	outfld_;
};

#endif

#ifndef uielasticmodelprov_h
#define uielasticmodelprov_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene
 Date:		Feb 2014
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "uigroup.h"
#include "uistring.h"

class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiVelSel;

mExpClass(uiSeis) uiElasticModelProvider : public uiGroup
{ mODTextTranslationClass(uiElasticModelProvider);
public:
			uiElasticModelProvider(uiParent*,bool is2d);
    bool		getInputMIDs(MultiID& pwmid,MultiID& swmid,
				     MultiID& aimid,MultiID& simid,
				     MultiID& denmid) const;

    uiString		errMsg() const			{ return errmsg_; }

protected:

    void		inpTypeSel(CallBacker*);
    void		sourceSel(CallBacker*);

    uiGenInput* 	inptypefld_;
    uiGenInput* 	inpsourceacfld_;
    uiGenInput* 	inpsourceelfld_;
    uiSeisSel*		optdensityfld_;
    uiSeisSel*		densityfld_;
    uiSeisSel*		aifld_;
    uiSeisSel*		sifld_;
    uiVelSel*		pwavefld_;
    uiVelSel*		swavefld_;
    uiIOObjSel* 	waveletfld_;
    uiString		errmsg_;

};

#endif

#pragma once
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
    bool		getInputMIDs(DBKey& pwmid,DBKey& swmid,
				     DBKey& aimid,DBKey& simid,
				     DBKey& denmid) const;
    void		setInputMIDs(const DBKey& pwmid,const DBKey& swmid,
				     const DBKey& aimid,const DBKey& simid,
				     const DBKey& denmid);

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

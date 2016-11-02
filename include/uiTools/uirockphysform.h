#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "propertyref.h"
class uiComboBox;
class uiGenInput;
class uiLabel;
class uiPushButton;
class uiTextEdit;
class uiTextBrowser;
class uiRockPhysConstantFld;
namespace Math { class Formula; }
namespace RockPhysics { class Formula; }


mExpClass(uiTools) uiRockPhysForm : public uiGroup
{ mODTextTranslationClass(uiRockPhysForm);
public:

			uiRockPhysForm(uiParent*);
			uiRockPhysForm(uiParent*,PropertyRef::StdType);

    PropertyRef::StdType getType() const;
    void		setType(PropertyRef::StdType);
				//!< only works when 1st constructor used

    bool		getFormulaInfo(Math::Formula&,
				    TypeSet<PropertyRef::StdType>* tps=0) const;
    const char*		getText(bool replace_consts=true) const;

    const uiString&	errMsg() const		{ return errmsg_; }
    bool		isOK();

protected:

    uiComboBox*				typfld_;
    EnumDefImpl<PropertyRef::StdType>	types_;
    uiComboBox*				nmfld_;
    uiTextEdit*				formulafld_;
    uiTextBrowser*			descriptionfld_;
    ObjectSet<uiRockPhysConstantFld>	cstflds_;

    const PropertyRef::StdType		fixedtype_;
    uiString				errmsg_;

    void		typSel(CallBacker*);
    void		nameSel(CallBacker*);

    void		createFlds(uiGroup*);
    BufferString	getFormText(const RockPhysics::Formula&,bool) const;

};

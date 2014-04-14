#ifndef uirockphysform_h
#define uirockphysform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id$
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
class uiRockPhysConstantFld;
namespace Math { class Formula; }
namespace RockPhysics { class Formula; }


mExpClass(uiTools) uiRockPhysForm : public uiGroup
{
public:

			uiRockPhysForm(uiParent*);
			uiRockPhysForm(uiParent*,PropertyRef::StdType);

    PropertyRef::StdType getType() const;
    void		setType(PropertyRef::StdType);
				//!< only works when 1st constructor used

    bool		getFormulaInfo(Math::Formula&,
				    TypeSet<PropertyRef::StdType>* tps=0) const;
    const char*		getText(bool replace_consts=true) const;

    const char*		errMsg() const		{ return errmsg_.buf(); }
    bool		isOK();

protected:

    uiComboBox*		typfld_;
    uiComboBox*		nmfld_;
    uiTextEdit*		formulafld_;
    uiTextEdit*		descriptionfld_;
    ObjectSet<uiRockPhysConstantFld>	cstflds_;

    const PropertyRef::StdType fixedtype_;
    BufferString	errmsg_;

    void		typSel(CallBacker*);
    void		nameSel(CallBacker*);

    void		createFlds(uiGroup*);
    BufferString	getFormText(const RockPhysics::Formula&,bool) const;

};


#endif

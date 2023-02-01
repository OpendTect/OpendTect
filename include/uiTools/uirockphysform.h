#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "mnemonics.h"

class uiComboBox;
class uiMnemonicsSel;
class uiRockPhysConstantFld;
class uiTextBrowser;
class uiTextEdit;
namespace Math { class Formula; }
namespace RockPhysics { class Formula; }


mExpClass(uiTools) uiRockPhysForm : public uiGroup
{ mODTextTranslationClass(uiRockPhysForm);
public:

			uiRockPhysForm(uiParent*);
			uiRockPhysForm(uiParent*,
				       Mnemonic::StdType);
			uiRockPhysForm(uiParent*,const Mnemonic&);
			~uiRockPhysForm();

    uiRetVal		isOK() const;

    const Mnemonic&	getMnemonic() const;

    const char*		getText(bool replace_consts=true) const;
    bool		getFormulaInfo(Math::Formula&) const;

    mDeprecated("Use MnemonicSelection")
    bool		getFormulaInfo(Math::Formula&,
				       TypeSet<Mnemonic::StdType>*) const;

private:

    uiComboBox*		typfld_ = nullptr;
    ObjectSet<uiMnemonicsSel> mnselflds_;
    uiComboBox*		nmfld_;
    uiTextEdit*		formulafld_;
    uiTextBrowser*	descriptionfld_;
    ObjectSet<uiRockPhysConstantFld>	cstflds_;

    const Mnemonic*	fixedmn_ = nullptr;

    void		initGrp(CallBacker*);
    void		typSel(CallBacker*);
    void		mnSel(CallBacker*);
    void		nameSel(CallBacker*);

    void		createMnemonicFld(const ObjectSet<MnemonicSelection>&,
					  uiObject* attachobj);
    void		createVolumeFlds(const Mnemonic&);
    void		createFlds(uiObject*);
    void		setType(const Mnemonic&);
    BufferString	getFormText(const RockPhysics::Formula&,bool) const;

};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;
class uiIOObjSel;
class uiCheckBox;


/*! \brief Horizon attribute description editor */

mClass(uiEMAttrib) uiHorizonAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiHorizonAttrib);
public:

			uiHorizonAttrib(uiParent*,bool);
			~uiHorizonAttrib();

protected:

    uiAttrSel*		inpfld_;
    uiIOObjSel*		horfld_;
    uiGenInput*		typefld_;
    uiGenInput*		surfdatafld_;
    uiCheckBox*		isrelbox_;

    BufferStringSet	surfdatanms_;
    int			nrouttypes_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    void		horSel(CallBacker*);
    void		typeSel(CallBacker*);

			mDeclReqAttribUIFns
};

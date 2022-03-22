#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
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

    virtual bool	setParameters(const Attrib::Desc&);
    virtual bool	setInput(const Attrib::Desc&);
    virtual bool	getParameters(Attrib::Desc&);
    virtual bool	getInput(Attrib::Desc&);

    void		horSel(CallBacker*);
    void		typeSel(CallBacker*);

			mDeclReqAttribUIFns
};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

class uiFileInput;
class uiIOObjSel;
class uiTextEdit;
class uiFileInput;
namespace Attrib { class DescSet; }

/*!
\brief
*/

mExpClass(uiAttributes) uiGetFileForAttrSet : public uiDialog
{ mODTextTranslationClass(uiGetFileForAttrSet);
public:
			uiGetFileForAttrSet(uiParent*,bool isads,bool is2d);
			~uiGetFileForAttrSet();

    const char*		fileName() const		{ return fname_; }
    Attrib::DescSet&	attrSet()			{ return attrset_; }

protected:

    uiFileInput*	fileinpfld;
    uiTextEdit*		infofld;
    BufferString	fname_;
    Attrib::DescSet&	attrset_;
    bool		isattrset_;

    void		srchDir(CallBacker*);
    void		selChg(CallBacker* =0);
    bool		acceptOK(CallBacker*) override;

};


/*!
\brief
*/

mExpClass(uiAttributes) uiImpAttrSet : public uiDialog
{ mODTextTranslationClass(uiImpAttrSet);
public:
			uiImpAttrSet(uiParent*);
			~uiImpAttrSet();

protected:
    void		inpChgd(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiFileInput*	fileinpfld_;
    uiIOObjSel*		attrsetfld_;
};

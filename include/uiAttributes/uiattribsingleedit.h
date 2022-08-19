#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "datapack.h"

namespace Attrib { class Desc; class DescSetMan; };
class uiAttrDescEd;
class uiGenInput;


/*! \brief Dialog for creating volume output */

mExpClass(uiAttributes) uiSingleAttribEd : public uiDialog
{ mODTextTranslationClass(uiSingleAttribEd);
public:

			uiSingleAttribEd(uiParent*,Attrib::Desc&,bool isnew);
			~uiSingleAttribEd();

    void		setDataPackSelection(const TypeSet<DataPack::FullID>&);

    bool		anyChange() const	{ return anychg_; }
    bool		nameChanged() const	{ return nmchgd_; }

protected:

    Attrib::Desc&	desc_;
    Attrib::DescSetMan*	setman_;
    bool		nmchgd_;
    bool		anychg_;

    uiAttrDescEd*	desced_;
    uiGenInput*		namefld_;

    bool		acceptOK(CallBacker*) override;

};

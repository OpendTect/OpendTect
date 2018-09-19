#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"
#include "datapack.h"

namespace Attrib { class Desc; class DescSetMan; };
class uiAttrDescEd;
class uiGenInput;


/*! \brief edits a single Attrib::Desc */

mExpClass(uiAttributes) uiSingleAttribEd : public uiDialog
{ mODTextTranslationClass(uiSingleAttribEd);
public:

			uiSingleAttribEd(uiParent*,Attrib::Desc&,bool isnew,
				     const TypeSet<DataPack::FullID>* dpids=0);
			~uiSingleAttribEd();

    bool		anyChange() const	{ return anychg_; }
    bool		nameChanged() const	{ return nmchgd_; }

protected:

    Attrib::Desc&	desc_;
    Attrib::DescSetMan*	setman_;
    bool		nmchgd_;
    bool		anychg_;

    uiAttrDescEd*	desced_;
    uiGenInput*		namefld_;

    bool		acceptOK();

};

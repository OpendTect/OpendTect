#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id$
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

    bool		acceptOK(CallBacker*);

};


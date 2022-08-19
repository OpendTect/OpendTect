#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

namespace Attrib { class DescSet; class SelSpec; }
class uiAttrSel;


mExpClass(uiAttributes) uiRGBAttrSelDlg : public uiDialog
{ mODTextTranslationClass(uiRGBAttrSelDlg);
public:
			uiRGBAttrSelDlg(uiParent*,const Attrib::DescSet&,
					Pos::GeomID);
			~uiRGBAttrSelDlg();

    void		setSelSpec(const TypeSet<Attrib::SelSpec>&);
    void		fillSelSpec(TypeSet<Attrib::SelSpec>&) const;

protected:

    bool		acceptOK(CallBacker*) override;

    uiAttrSel*		rfld_;
    uiAttrSel*		gfld_;
    uiAttrSel*		bfld_;
    uiAttrSel*		tfld_;

};

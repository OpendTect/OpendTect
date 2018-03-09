#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2016
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

namespace Attrib { class DescSet; class SelSpec; class SelSpecList; }
class uiAttrSel;


mExpClass(uiAttributes) uiRGBAttrSelDlg : public uiDialog
{ mODTextTranslationClass(uiRGBAttrSelDlg);
public:
			uiRGBAttrSelDlg(uiParent*,const Attrib::DescSet&);
			~uiRGBAttrSelDlg();

    void		setSelSpec(const Attrib::SelSpecList&);
    void		fillSelSpec(Attrib::SelSpecList&) const;

protected:

    bool		acceptOK();

    uiAttrSel*		rfld_;
    uiAttrSel*		gfld_;
    uiAttrSel*		bfld_;
    uiAttrSel*		tfld_;

};

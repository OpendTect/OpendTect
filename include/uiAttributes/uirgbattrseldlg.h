#ifndef uirgbattrseldlg_h
#define uirgbattrseldlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2016
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uidialog.h"

namespace Attrib { class DescSet; class SelSpec; }
class uiAttrSel;


mExpClass(uiAttributes) uiRGBAttrSelDlg : public uiDialog
{ mODTextTranslationClass(uiRGBAttrSelDlg);
public:
			uiRGBAttrSelDlg(uiParent*,const Attrib::DescSet&);
			~uiRGBAttrSelDlg();

    void		setSelSpec(const TypeSet<Attrib::SelSpec>&);
    void		fillSelSpec(TypeSet<Attrib::SelSpec>&) const;

protected:

    bool		acceptOK(CallBacker*);

    uiAttrSel*		rfld_;
    uiAttrSel*		gfld_;
    uiAttrSel*		bfld_;
    uiAttrSel*		tfld_;

};

#endif

#ifndef uiattribsingleedit_h
#define uiattribsingleedit_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uiattribsingleedit.h,v 1.1 2011-01-06 16:19:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

namespace Attrib { class Desc; class DescSetMan; };
class uiAttrDescEd;
class uiGenInput;


/*! \brief Dialog for creating volume output */

mClass uiSingleAttribEd : public uiDialog
{
public:

			uiSingleAttribEd(uiParent*,Attrib::Desc&,bool isnew);
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

    bool		acceptOK(CallBacker*);

};

#endif

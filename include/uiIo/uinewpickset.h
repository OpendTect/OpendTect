#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"
#include "pickset.h"

class uiGenInput;
class uiMarkerStyle3D;


/*!\brief dialog for creating an empty new Pick::Set, or base class for one
  that puts stuff in it. */


mExpClass(uiIo) uiNewPickSetDlg : public uiDialog
{ mODTextTranslationClass(uiNewPickSetDlg);
public:

			uiNewPickSetDlg(uiParent*,bool poly,const char* cat=0);

    RefMan<Pick::Set>	getSet() const		    { return set_; }

protected:

    uiGenInput*		nmfld_;
    uiMarkerStyle3D*	markerstylefld_;

    const bool		ispolygon_;
    const BufferString	category_;
    RefMan<Pick::Set>	set_;

    void		attachStdFlds(bool mineabove,uiGroup* mygrp);
    virtual bool	fillData(Pick::Set&)	    { return true; }

    bool		acceptOK();
    RefMan<Pick::Set>	getEmptyPickSet() const;

};

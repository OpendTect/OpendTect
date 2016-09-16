#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2015
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "iopar.h"
#include "uistring.h"
class uiParent;
class uiSEGYRead;


mExpClass(uiSEGY) uiSEGYTool : public CallBacker
{ mODTextTranslationClass(uiSEGYTool);

			uiSEGYTool(uiParent*,IOPar* previop=0,int choice=-1);

    void		go();

protected:

    uiParent*		parent_;
    bool		isnext_;
    IOPar		pars_;
    int			choice_;

    uiSEGYRead*		segyread_;

    bool		launchSEGYWiz();
    void		doVSPTool();

    void		toolEnded(CallBacker*);

};

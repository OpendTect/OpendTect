#ifndef uisegytool_h
#define uisegytool_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2015
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"


mExpClass(uiSEGY) uiSEGYTool : public CallBacker
{

			uiSEGYTool(uiParent*,IOPar* previop=0,int choice=-1);

    bool		go();

protected:

    uiParent*		parent_;
    bool		isnext_;
    IOPar		pars_;
    int			choice_;

    uiSEGYRead*		segyread_;

    bool		doVSPTool(IOPar*,int);
    bool		launchSEGYWiz(IOPar*,int);

    void		toolEnded(CallBacker*);

};


#endif

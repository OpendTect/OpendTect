#ifndef uiseispreloadmgr_h
#define uiseispreloadmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uisynthtorealscale.h,v 1.1 2011-02-07 10:25:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
class SeisTrcBuf;
class uiStratSeisEvent;


/*!\brief To determine scaling of synthetics using real data.
 
  Note: the input trc buf *must* have ref times in the trc.info().pick's.

 */

mClass uiSynthToRealScale : public uiDialog
{ 	
public:
			uiSynthToRealScale(uiParent*,SeisTrcBuf&,
					   const MultiID& wvltid,
					   const char* reflvlnm);

protected:

    SeisTrcBuf&		synth_;
    MultiID		wvltid_;

    uiStratSeisEvent*	evfld_;

    void		initWin(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif

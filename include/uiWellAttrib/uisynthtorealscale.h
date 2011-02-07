#ifndef uiseispreloadmgr_h
#define uiseispreloadmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uisynthtorealscale.h,v 1.2 2011-02-07 16:17:43 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
class SeisTrcBuf;
class uiSeisSel;
class uiIOObjSel;
class uiGenInput;
class uiStratSeisEvent;
class uiSynthToRealScaleStatsDisp;


/*!\brief To determine scaling of synthetics using real data.
 
  Note: the input trc buf *must* have ref times in the trc.info().pick's.

 */

mClass uiSynthToRealScale : public uiDialog
{ 	
public:

			uiSynthToRealScale(uiParent*,bool is2d,SeisTrcBuf&,
					   const MultiID& wvltid,
					   const char* reflvlnm);

protected:

    bool		is2d_;
    SeisTrcBuf&		synth_;
    MultiID		wvltid_;

    uiSeisSel*		seisfld_;
    uiIOObjSel*		horfld_;
    uiIOObjSel*		polyfld_;
    uiIOObjSel*		wvltfld_;
    uiStratSeisEvent*	evfld_;
    uiGenInput*		finalscalefld_;
    uiSynthToRealScaleStatsDisp*	synthstatsfld_;
    uiSynthToRealScaleStatsDisp*	realstatsfld_;

    void		initWin(CallBacker*);
    void		setScaleFld(CallBacker*);
    void		goPush( CallBacker* )
    			{ updSynthStats(); updRealStats(); }
    bool		acceptOK(CallBacker*);

    void		updSynthStats();
    void		updRealStats();

};


#endif

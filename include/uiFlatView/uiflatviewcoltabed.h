#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		May 2008
________________________________________________________________________


-*/

#include "uiflatviewmod.h"
#include "callback.h"
#include "flatview.h"

class uiColTabToolBar;
class uiColTabSelTool;


/*!\brief FlatView color table editor. */

mExpClass(uiFlatView) uiFlatViewColTabEd : public CallBacker
{
public:

    typedef FlatView::DataDispPars::VD	VDDispPars;

			uiFlatViewColTabEd(uiColTabToolBar&);
			~uiFlatViewColTabEd();

    uiColTabSelTool&	selTool()		{ return ctseltool_; }
    void		setSensitive(bool yn);

    const VDDispPars&	displayPars() const		{ return vdpars_; }
    VDDispPars&		displayPars()			{ return vdpars_; }
    void		setDisplayPars(const VDDispPars&);

    Notifier<uiFlatViewColTabEd> refreshReq;
    Notifier<uiFlatViewColTabEd> colTabChgd;

protected:

    uiColTabSelTool&		ctseltool_;
    FlatView::DataDispPars::VD	vdpars_;

    void			mapperChgCB(CallBacker*);
    void			seqChgCB(CallBacker*);
    void			refreshReqCB(CallBacker*);

};

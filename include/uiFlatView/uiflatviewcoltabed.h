#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "callback.h"
#include "flatview.h"

class uiColorTableToolBar;

/*!
\brief FlatView color table editor.
*/

mExpClass(uiFlatView) uiFlatViewColTabEd : public CallBacker
{
public:
			uiFlatViewColTabEd(uiColorTableToolBar&);
			~uiFlatViewColTabEd();

    void		setColTab(const FlatView::DataDispPars::VD&);
    void		setSensitive(bool yn);

    FlatView::DataDispPars::VD&		getDisplayPars()
					{ return vdpars_; }
    Notifier<uiFlatViewColTabEd>	colTabChgd;

protected:

    uiColorTableToolBar&	uicoltab_;
    FlatView::DataDispPars::VD	vdpars_;

    void			colTabChanged(CallBacker*);
};

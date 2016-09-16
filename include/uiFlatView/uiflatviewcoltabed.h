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

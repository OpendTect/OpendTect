#ifndef uibasemapcoltabed_h
#define uibasemapcoltabed_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		May 2008
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uibasemapmod.h"
#include "callback.h"
#include "flatview.h"

class uiColorTableToolBar;

/*!
\brief FlatView color table editor.
*/

mExpClass(uiBasemap) uiBasemapColTabEd : public CallBacker
{
public:
			uiBasemapColTabEd(uiColorTableToolBar&);
			~uiBasemapColTabEd();

    void		setColTab(const FlatView::DataDispPars::VD&);
    void		setSensitive(bool yn);
    FlatView::DataDispPars::VD& getDisplayPars() { return vdpars_; }

    Notifier<uiBasemapColTabEd> colTabChgd;

protected:

    uiColorTableToolBar&	uicoltab_;
    FlatView::DataDispPars::VD	vdpars_;

    void			colTabChanged(CallBacker*);
};

#endif


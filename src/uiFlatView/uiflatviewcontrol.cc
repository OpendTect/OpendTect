/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewcontrol.cc,v 1.1 2007-02-19 16:41:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewcontrol.h"
#include "uiflatviewer.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "pixmap.h"

#define mDefBut(butnm,grp,fnm,cbnm,tt) \
    butnm = new uiToolButton( grp, 0, ioPixmap(fnm), \
			      mCB(this,uiFlatViewControl,cbnm) ); \
    butnm->setToolTip( tt )

uiFlatViewControl::uiFlatViewControl( uiFlatViewer& fv, const Setup& s )
    : uiGroup(fv.attachObj()->parent(),"Flat viewer control")
    , vwr_(fv)
    , setup_(s)
    , posChange(this)
    , stategrp_(0)
{
    setBorder( 0 );

    if ( setup_.withstates_ )
    {
	stategrp_ = new uiButtonGroup( this, "", setup_.vertical_ );
	mDefBut(manipbut_,stategrp_,"view.png",stateCB,"View mode (zoom)");
	mDefBut(drawbut_,stategrp_,"pick.png",stateCB,"Interact mode");
    }

    posgrp_ = new uiButtonGroup( this, "", setup_.vertical_ );
    mDefBut(zoominbut_,posgrp_,"zoomforward.png",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,posgrp_,"zoombackward.png",zoomCB,"Zoom out");
    mDefBut(panupbut_,posgrp_,"uparrow.png",panCB,"Pan up");
    mDefBut(panleftbut_,posgrp_,"leftarrow.png",panCB,"Pan left");
    mDefBut(panrightbut_,posgrp_,"rightarrow.png",panCB,"Pan right");
    mDefBut(pandownbut_,posgrp_,"downarrow.png",panCB,"Pan down");

    parsgrp_ = new uiButtonGroup( this, "", setup_.vertical_ );
    mDefBut(parsbut_,parsgrp_,"2ddisppars.png",parsCB,"Set display parameters");

    if ( stategrp_ )
	posgrp_->attach( setup_.vertical_?ensureBelow:ensureRightOf, stategrp_);
    parsgrp_->attach( setup_.vertical_ ? ensureBelow : ensureRightOf, posgrp_ );

    if ( setup_.vertical_ )
	vwr_.attach( rightOf, this );
    else
	attach( alignedBelow, &vwr_ );

}


uiFlatViewControl::~uiFlatViewControl()
{
}


void uiFlatViewControl::zoomCB( CallBacker* but )
{
}


void uiFlatViewControl::panCB( CallBacker* but )
{
}


void uiFlatViewControl::stateCB( CallBacker* but )
{
}


void uiFlatViewControl::parsCB( CallBacker* )
{
}

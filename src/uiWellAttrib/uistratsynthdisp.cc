/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthdisp.cc,v 1.1 2010-12-01 16:56:38 cvsbert Exp $";

#include "uistratsynthdisp.h"
#include "uitoolbutton.h"
#include "uicombobox.h"
#include "uigraphicsview.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "wavelet.h"
#include "aimodel.h"
#include "iodirentry.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "ioman.h"


uiStratSynthDisp::uiStratSynthDisp( uiParent* p,
						const Strat::LayerModel& lm )
    : uiGroup(p,"LayerModel synthetics display")
    , lm_(lm)
{
    const CallBack redrawcb( mCB(this,uiStratSynthDisp,reDraw) );
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Wavelet" );
    wvltfld_ = lcb->box();
    fillWvltField();
    wvltfld_->selectionChanged.notify( redrawcb );
    uiToolButton* tb = new uiToolButton( this, "man_wvlt.png",
	    				 "Manage wavelets",
	    				 mCB(this,uiStratSynthDisp,manWvlts) );
    tb->attach( rightOf, lcb );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
}


void uiStratSynthDisp::reDraw( CallBacker* )
{
}


void uiStratSynthDisp::manWvlts( CallBacker* )
{
    // pop up man wvlts
    // redraw if wvlt chgd (name or content)
    fillWvltField();
}


bool uiStratSynthDisp::fillWvltField()
{
    IOObjContext ctxt( mIOObjContext(Wavelet) );
    IOM().to( ctxt.getSelKey() );
    IODirEntryList dil( IOM().dirPtr(), ctxt );
    BufferStringSet nms;
    for ( int idx=0; idx<dil.size(); idx ++ )
	nms.add( dil[idx]->ioobj->name() );

    BufferString curwvlt( wvltfld_->text() );
    wvltfld_->setEmpty();
    wvltfld_->addItems( nms );
    int newidx = nms.indexOf( curwvlt.buf() );
    bool ispresent = newidx >= 0;
    if ( curwvlt.isEmpty() || !ispresent )
    {
	const char* res = SI().pars().find(
		IOPar::compKey(sKey::Default,ctxt.trgroup->userName()) );
	if ( res && *res )
	{
	    IOObj* ioobj = IOM().get( MultiID(res) );
	    if ( ioobj )
		{ curwvlt = ioobj->name(); delete ioobj; }
	}
    }
    newidx = nms.indexOf( curwvlt.buf() ); ispresent = newidx >= 0;

    if ( ispresent )
	wvltfld_->setCurrentItem( newidx );
    return ispresent;
}


void uiStratSynthDisp::modelChanged()
{
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiseiswvltsel.h"
#include "uiseiswvltman.h"
#include "uicombobox.h"
#include "uitoolbutton.h"
#include "wavelet.h"
#include "iodirentry.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "ioman.h"


uiSeisWaveletSel::uiSeisWaveletSel( uiParent* p, const char* seltxt )
    : uiGroup(p,"Wavelet selector")
    , newSelection(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
	    					seltxt ? seltxt : "Wavelet" );
    nmfld_ = lcb->box();

    uiToolButton* tb = new uiToolButton( this, "man_wvlt",
	    "Manage wavelets", mCB(this,uiSeisWaveletSel,startMan) );

    tb->attach( rightOf, lcb );
    setHAlignObj( lcb );

    rebuildList();
    initFlds( this );
    setFrame( true );
}


uiSeisWaveletSel::~uiSeisWaveletSel()
{
    deepErase( ids_ );
}


void uiSeisWaveletSel::initFlds( CallBacker* )
{
    nmfld_->selectionChanged.notify( mCB(this,uiSeisWaveletSel,selChg) );
}


void uiSeisWaveletSel::setInput( const char* nm )
{
    if ( nm && *nm )
    {
	if ( !nmfld_->isPresent(nm) )
	    rebuildList();
	nmfld_->setText( nm );
    }
}


void uiSeisWaveletSel::setInput( const MultiID& mid )
{
    if ( mid.isEmpty() ) return;

    IOObj* ioobj = IOM().get( mid );
    if ( !ioobj ) return;

    setInput( ioobj->name() );
    delete ioobj;
}


const char* uiSeisWaveletSel::getName() const
{
    return nmfld_->text();
}


const MultiID& uiSeisWaveletSel::getID() const
{
    static const MultiID emptyid;
    const int selidx = nmfld_->currentItem();
    return selidx < 0 ? emptyid : *ids_[selidx];
}


Wavelet* uiSeisWaveletSel::getWavelet() const
{
    const MultiID& id = getID();
    if ( id.isEmpty() ) return 0;
    IOObj* ioobj = IOM().get( id );
    if ( !ioobj ) return 0;
    Wavelet* ret = Wavelet::get( ioobj );
    delete ioobj;
    return ret;
}


void uiSeisWaveletSel::startMan( CallBacker* )
{
    uiSeisWvltMan dlg( this );
    dlg.go();
    rebuildList();
    selChg( nmfld_ );
}


void uiSeisWaveletSel::selChg( CallBacker* )
{
    newSelection.trigger();
}


void uiSeisWaveletSel::rebuildList()
{
    IOObjContext ctxt( mIOObjContext(Wavelet) );
    IOM().to( ctxt.getSelKey() );
    IODirEntryList dil( IOM().dirPtr(), ctxt );
    nms_.erase(); deepErase( ids_ );
    for ( int idx=0; idx<dil.size(); idx ++ )
    {
	nms_.add( dil[idx]->ioobj->name() );
	ids_ += new MultiID( dil[idx]->ioobj->key() );
    }

    BufferString curwvlt( nmfld_->text() );
    nmfld_->selectionChanged.disable();

    nmfld_->setEmpty(); nmfld_->addItems( nms_ );

    int newidx = nms_.indexOf( curwvlt.buf() );
    if ( curwvlt.isEmpty() || newidx < 0 )
    {
	const char* res = SI().pars().find(
		IOPar::compKey(sKey::Default(),ctxt.trgroup->userName()) );
	if ( res && *res )
	{
	    IOObj* ioobj = IOM().get( MultiID(res) );
	    if ( ioobj )
	    {
		curwvlt = ioobj->name();
		newidx = nms_.indexOf( curwvlt.buf() );
		delete ioobj;
		nmfld_->selectionChanged.enable();
	    }
	}
    }

    if ( newidx >= 0 )
	nmfld_->setCurrentItem( newidx );
    nmfld_->selectionChanged.enable();
}

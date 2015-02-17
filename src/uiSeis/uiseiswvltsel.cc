/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiswvltsel.h"

#include "uicombobox.h"
#include "uiseiswvltman.h"
#include "uistrings.h"
#include "uitoolbutton.h"
#include "uiwaveletextraction.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletio.h"


uiSeisWaveletSel::uiSeisWaveletSel( uiParent* p, const char* seltxt,
                                    bool withextract, bool withman )
    : uiGroup(p,"Wavelet selector")
    , wvltextrdlg_(0)
    , newSelection(this)
{
    uiLabeledComboBox* lcb =
	new uiLabeledComboBox( this, seltxt ? seltxt : uiStrings::sWavelet() );
    nmfld_ = lcb->box();
    uiObject* lastfld = lcb->attachObj();

    if ( withextract )
    {
	uiPushButton* crwvltbut = new uiPushButton( this, tr("Extract"),
				mCB(this,uiSeisWaveletSel,extractCB), false );
	crwvltbut->attach( rightOf, lastfld );
	lastfld = crwvltbut;
    }

    if ( withman )
    {
	uiToolButton* tb = new uiToolButton( this, "man_wvlt",
		uiStrings::sManWav(), mCB(this,uiSeisWaveletSel,startMan) );
	tb->attach( rightOf, lastfld );
    }

    rebuildList();
    initFlds( this );
    setHAlignObj( lcb );
}


uiSeisWaveletSel::~uiSeisWaveletSel()
{
    deepErase( ids_ );
    if ( wvltextrdlg_ ) wvltextrdlg_->close();
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


void uiSeisWaveletSel::extractCB( CallBacker* )
{
    if ( !wvltextrdlg_ )
    {
	wvltextrdlg_ = new uiWaveletExtraction( this, false );
	wvltextrdlg_->extractionDone.notify(
		mCB(this,uiSeisWaveletSel,extractionDoneCB) );
    }

    wvltextrdlg_->show();
}


void uiSeisWaveletSel::extractionDoneCB( CallBacker* )
{
    rebuildList();
    setInput( wvltextrdlg_->storeKey() );
}


void uiSeisWaveletSel::startMan( CallBacker* )
{
    uiSeisWvltMan dlg( this );
    dlg.setModal( true );
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
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList dil( iodir, ctxt );
    nms_.erase(); deepErase( ids_ );
    for ( int idx=0; idx<dil.size(); idx ++ )
    {
	nms_.add( dil[idx]->ioobj_->name() );
	ids_ += new MultiID( dil[idx]->ioobj_->key() );
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


// uiWaveletSel
#define mSelTxt(seltxt) !seltxt.isEmpty() ? \
	seltxt : ( forread ? "Input Wavelet" : "Output Wavelet" )

uiWaveletSel::uiWaveletSel( uiParent* p, bool forread,
			    const uiIOObjSel::Setup& setup )
    : uiIOObjSel(p,mRWIOObjContext(Wavelet,forread),setup)
{
    if ( setup.seltxt_.isEmpty() )
	setLabelText( forread ? tr("Input Wavelet") : tr("Output Wavelet") );
    fillEntries();
}


Wavelet* uiWaveletSel::getWavelet( bool noerr ) const
{
    const IOObj* selioobj = ioobj( noerr );
    return selioobj ? Wavelet::get( selioobj ) : 0;
}

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiswvltsel.h"

#include "uicombobox.h"
#include "uimsg.h"
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


uiSeisWaveletSel::uiSeisWaveletSel( uiParent* p, const Setup& su )
    : uiSeisWaveletSel(p,su.seltxt_.buf(),su.withextract_,
		       su.withman_,su.compact_)
{
}


uiSeisWaveletSel::uiSeisWaveletSel( uiParent* p, const char* seltxt,
				bool withextract, bool withman, bool compact )
    : uiGroup(p,"Wavelet selector")
    , newSelection(this)
{
    auto* lcb = new uiLabeledComboBox( this,
		seltxt ? mToUiStringTodo(seltxt) : uiStrings::sWavelet() );
    nmfld_ = lcb->box();
    nmfld_->setHSzPol( uiObject::Wide );
    uiObject* lastfld = lcb->attachObj();

    if ( withman )
    {
	uiToolButtonSetup tbsu( "man_wvlt",
			uiStrings::phrManage(uiStrings::sWavelet(mPlural)),
			mCB(this,uiSeisWaveletSel,startMan) );
	uiButton* but;
	if ( compact )
	    but = tbsu.getToolButton( this );
	else
	    but = tbsu.getPushButton( this );
	but->attach( rightOf, lastfld );
	lastfld = but;
    }

    if ( withextract )
    {
	uiToolButtonSetup tbsu( "wavelet_extract", tr("Extract"),
				mCB(this,uiSeisWaveletSel,extractCB) );
	uiButton* but;
	if ( compact )
	    but = tbsu.getToolButton( this );
	else
	    but = tbsu.getPushButton( this );
	but->attach( rightOf, lastfld );
	lastfld = but;
    }

    rebuildList();
    initFlds( this );
    setHAlignObj( lcb );
}


uiSeisWaveletSel::~uiSeisWaveletSel()
{
    detachAllNotifiers();

    deepErase( ids_ );
    if ( wvltextrdlg_ )
	wvltextrdlg_->close();
}


void uiSeisWaveletSel::initFlds( CallBacker* )
{
    mAttachCB( nmfld_->selectionChanged, uiSeisWaveletSel::selChg );
}


void uiSeisWaveletSel::setInput( const char* nm )
{
    const StringView nmstr = nm;
    if ( nmstr.isEmpty() )
	return;

    if ( !nmfld_->isPresent(nm) )
	rebuildList();

    nmfld_->setText( nm );
}


void uiSeisWaveletSel::setInput( const MultiID& mid )
{
    if ( mid.isUdf() )
	return;

    ConstPtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return;

    setInput( ioobj->name().buf() );
}


const MultiID& uiSeisWaveletSel::getID() const
{
    static const MultiID emptyid = MultiID::udf();
    const int selidx = nmfld_->currentItem();
    return selidx < 0 ? emptyid : *ids_[selidx];
}


Wavelet* uiSeisWaveletSel::getWavelet() const
{
    const MultiID& mid = getID();
    if ( mid.isUdf() )
	return nullptr;

    ConstPtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return nullptr;

    Wavelet* ret = Wavelet::get( ioobj.ptr() );
    return ret;
}


const char* uiSeisWaveletSel::getWaveletName() const
{
    return nmfld_->text();
}


void uiSeisWaveletSel::extractCB( CallBacker* )
{
    deleteAndNullPtr(wvltextrdlg_);
    bool for2d = SI().has2D();
    if ( for2d && SI().has3D() )
    {
	const int res = uiMSG().ask2D3D( tr("Use 2D or 3D data?"), true );
	if ( res == -1 )
	    return;
	else
	    for2d = res == 1;
    }

    wvltextrdlg_ = new uiWaveletExtraction( this, for2d );
    mAttachCB( wvltextrdlg_->extractionDone,
	       uiSeisWaveletSel::extractionDoneCB );
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
    const IOObjContext ctxt( mIOObjContext(Wavelet) );
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList dil( iodir, ctxt );
    nms_.erase();
    deepErase( ids_ );

    for ( int idx=0; idx<dil.size(); idx ++ )
    {
	nms_.add( dil[idx]->ioobj_->name() );
	ids_ += new MultiID( dil[idx]->ioobj_->key() );
    }

    BufferString curwvlt( nmfld_->text() );
    nmfld_->selectionChanged.disable();

    nmfld_->setEmpty();
    nmfld_->addItems( nms_ );

    int newidx = nms_.indexOf( curwvlt.buf() );
    if ( curwvlt.isEmpty() || newidx < 0 )
    {
	MultiID mid;
	SI().pars().get(
	    IOPar::compKey(sKey::Default(),ctxt.trgroup_->groupName()), mid );
	if ( !mid.isUdf() )
	{
	    ConstPtrMan<IOObj> ioobj = IOM().get( mid );
	    if ( ioobj )
	    {
		curwvlt = ioobj->name();
		newidx = nms_.indexOf( curwvlt.buf() );
		nmfld_->selectionChanged.enable();
	    }
	}
    }

    if ( newidx >= 0 )
	nmfld_->setCurrentItem( newidx );

    nmfld_->selectionChanged.enable();
}


// uiWaveletSel
uiWaveletSel::uiWaveletSel( uiParent* p, bool forread,
			    const uiIOObjSel::Setup& setup )
    : uiIOObjSel(p,mRWIOObjContext(Wavelet,forread),setup)
{
    if ( setup.seltxt_.isEmpty() )
	setLabelText( forread
		     ? uiStrings::phrInput( uiStrings::sWavelet() )
		     : uiStrings::phrOutput( uiStrings::sWavelet() ) );
    fillEntries();
}


uiWaveletSel::uiWaveletSel( uiParent* p, bool forread )
    : uiIOObjSel(p,mRWIOObjContext(Wavelet,forread),Setup())
{
    setLabelText( forread
		 ? uiStrings::phrInput( uiStrings::sWavelet() )
		 : uiStrings::phrOutput( uiStrings::sWavelet() ) );
    fillEntries();
}


uiWaveletSel::~uiWaveletSel()
{}


Wavelet* uiWaveletSel::getWavelet( bool noerr ) const
{
    const IOObj* selioobj = ioobj( noerr );
    return selioobj ? Wavelet::get( selioobj ) : 0;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2016
________________________________________________________________________

-*/

#include "uiwaveletsel.h"
#include "uiwaveletextraction.h"
#include "waveletmanager.h"
#include "waveletio.h"
#include "uitoolbutton.h"
#include "uiseiswvltman.h"
#include "uimsg.h"


IOObjContext uiWaveletIOObjSel::getCtxt( bool forread )
{
    IOObjContext ret( mIOObjContext(Wavelet) );
    ret.forread_ = forread;
    return ret;
}


uiWaveletIOObjSel::uiWaveletIOObjSel( uiParent* p, bool forread )
    : uiIOObjSel(p,getCtxt(forread))
{
    init( Setup(), forread );
}


uiWaveletIOObjSel::uiWaveletIOObjSel( uiParent* p, const Setup& su, bool forrd )
    : uiIOObjSel(p,getCtxt(forrd),su)
{
    init( su, forrd );
}


void uiWaveletIOObjSel::init( const Setup& su, bool forread )
{
    extrdlg_ = 0;
    if ( !forread )
	return;

    if ( su.withman_ )
    {
	uiToolButtonSetup tbsu( "man_wvlt",
		uiStrings::phrManage(uiStrings::sWavelet(mPlural)),
		mCB(this,uiWaveletIOObjSel,startManCB) );
	extbuts_ += tbsu.getToolButton( this );
    }

    if ( su.withextract_ )
    {
	uiToolButtonSetup tbsu( "wavelet_extract", uiStrings::sExtract(),
				mCB(this,uiWaveletIOObjSel,extractCB) );
	extbuts_ += tbsu.getToolButton( this );
    }
}


void uiWaveletIOObjSel::startManCB( CallBacker* )
{
    uiSeisWvltMan dlg( this );
    dlg.setModal( true );
    dlg.go();
    fullUpdate();
}


void uiWaveletIOObjSel::extractCB( CallBacker* )
{
    if ( !extrdlg_ )
    {
	extrdlg_ = new uiWaveletExtraction( this, false );
	mAttachCB( extrdlg_->extractionDone,
		   uiWaveletIOObjSel::extractionDoneCB );
	mAttachCB( extrdlg_->windowClosed,
		   uiWaveletIOObjSel::extrDlgCloseCB );
    }
    extrdlg_->show();
}


void uiWaveletIOObjSel::extrDlgCloseCB( CallBacker* )
{
    extrdlg_ = 0;
}


void uiWaveletIOObjSel::extractionDoneCB( CallBacker* )
{
    fullUpdate();
    setInput( extrdlg_->storeKey() );
}


ConstRefMan<Wavelet> uiWaveletIOObjSel::getWavelet() const
{
    const IOObj* wvltioobj = ioobj(true);
    if ( !wvltioobj )
	return 0;

    uiRetVal uirv;
    ConstRefMan<Wavelet> wvlt = WaveletMGR().fetch( wvltioobj->key(), uirv );
    uiMSG().handleErrors( uirv );
    return wvlt;
}


RefMan<Wavelet> uiWaveletIOObjSel::getWaveletForEdit() const
{
    const IOObj* wvltioobj = ioobj();
    if ( !wvltioobj )
	return 0;

    uiRetVal uirv;
    RefMan<Wavelet> wvlt = WaveletMGR().fetchForEdit( wvltioobj->key(), uirv );
    uiMSG().handleErrors( uirv );
    return wvlt;
}


bool uiWaveletIOObjSel::store( const Wavelet& wvlt, bool askoverwrite )
{
    const IOObj* wvltioobj = ioobj();
    if ( !wvltioobj )
	return 0;

    if ( askoverwrite )
    {
	ConstRefMan<Wavelet> existwvlt = WaveletMGR().fetch( wvltioobj->key() );
	if ( existwvlt && !existwvlt->isEmpty() )
	{
	    const int res = uiMSG().askOverwrite(
		    tr("A Wavelet with this name already exists.\nContinue?") );
	    if ( res < 1 )
		return false;
	}
    }

    SilentTaskRunnerProvider trprov;
    uiRetVal uirv = WaveletMGR().store( wvlt, wvltioobj->key(), trprov );
    if ( uirv.isError() )
	{ uiMSG().error( uirv ); return false; }

    return true;
}

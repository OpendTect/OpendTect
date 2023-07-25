/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiswvltimpexp.h"

#include "filepath.h"
#include "ioobj.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "tabledef.h"
#include "wavelet.h"
#include "waveletio.h"
#include "waveletattrib.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"
#include "uiseiswvltsel.h"
#include "od_helpids.h"

#include <math.h>


uiSeisWvltImp::uiSeisWvltImp( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Wavelet"),mNoDlgTitle,
				 mODHelpKey(mSeisWvltImpHelpID) ))
    , fd_(*WaveletAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::Table );
    mAttachCB( inpfld_->valueChanged, uiSeisWvltImp::inputChgd );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, inpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
		  mODHelpKey(mSeisWvltImpParsHelpID)  );
    dataselfld_->attach( alignedBelow, inpfld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld_ );

    scalefld_ = new uiGenInput( this, tr("Scale factor for samples"),
				FloatInpSpec(1) );
    scalefld_->attach( alignedBelow, dataselfld_ );
    scalefld_->attach( ensureBelow, sep );

    wvltfld_ = new uiWaveletSel( this, false );
    wvltfld_->attach( alignedBelow, scalefld_ );
}


uiSeisWvltImp::~uiSeisWvltImp()
{
    detachAllNotifiers();
    delete &fd_;
}


void uiSeisWvltImp::inputChgd( CallBacker* )
{
    const FilePath fnmfp( inpfld_->fileName() );
    wvltfld_->setInputText( fnmfp.baseName() );
}

#define mErrRet(s) { if ( (s).isSet() ) uiMSG().error(s); return false; }

bool uiSeisWvltImp::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( tr("Please enter the input file name") )

    const IOObj* wvltioobj = wvltfld_->ioobj( true );
    if ( !wvltioobj )
	mErrRet( !wvltfld_->isEmpty() ? uiString::emptyString()
		: tr("Please enter a name for the new wavelet") )
    if ( !dataselfld_->commit() )
	return false;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )

    WaveletAscIO aio( fd_ );
    PtrMan<Wavelet> wvlt = aio.get( strm );
    if ( !wvlt )
	mErrRet(aio.errMsg())

    WaveletAttrib wvltattrib( *wvlt );
    const bool quadraturewvlt = mIsEqual(wvltattrib.getAvgPhase(true),90.f,1.f);

    Interval<float> vals;
    wvlt->getExtrValues( vals );
    const float extreme = Math::Abs(vals.start) > Math::Abs(vals.stop)
				? vals.start : vals.stop;
    const int maxsamp = wvlt->getPos( quadraturewvlt ? 0.f : extreme,
				      quadraturewvlt );
    const int nrhdrlines = fd_.nrHdrLines();

    if ( maxsamp != wvlt->centerSample() )
    {
	uiString msg;
	msg = tr( "Center of wavelet is predicted at row number: %1" )
		.arg( maxsamp + 1 + nrhdrlines );
	msg.append( tr(" The provided center sample row position was: %1" )
			.arg( wvlt->centerSample() + 1 + nrhdrlines ), true );
	msg.append( "", true );
	msg.append( tr( "\n\nDo you want to reposition the center sample?" ) );
	if ( uiMSG().askGoOn(msg) )
	    wvlt->setCenterSample( maxsamp );
    }

    const float fac = scalefld_->getFValue();
    if ( !mIsUdf(fac) && !mIsZero(fac,mDefEpsF) && !mIsEqual(fac,1.f,mDefEpsF) )
	wvlt->transform( 0.f, fac );

    if ( !wvlt->put(wvltioobj) )
	mErrRet( tr("Cannot store wavelet on disk") )

    uiString msg = tr("Wavelet successfully imported."
		      "\n\nDo you want to import more Wavelets?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


MultiID uiSeisWvltImp::selKey() const
{
    return wvltfld_->key();
}


// uiSeisWvltExp
uiSeisWvltExp::uiSeisWvltExp( uiParent* p )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( uiStrings::sWavelet() ),
				  mNoDlgTitle,
				  mODHelpKey(mSeisWvltImpHelpID) ))
{
    setOkText( uiStrings::sExport() );

    wvltfld_ = new uiWaveletSel( this, true );
    mAttachCB( wvltfld_->selectionDone, uiSeisWvltExp::inputChgd );

    addzfld_ = new uiGenInput( this, uiStrings::phrOutput(SI().zIsTime() ?
				     uiStrings::sTime() : uiStrings::sDepth()),
				     BoolInpSpec(true) );
    addzfld_->attach( alignedBelow, wvltfld_ );

    outpfld_ = new uiASCIIFileInput( this, false );
    outpfld_->attach( alignedBelow, addzfld_ );
}


uiSeisWvltExp::~uiSeisWvltExp()
{
    detachAllNotifiers();
}


void uiSeisWvltExp::inputChgd( CallBacker* )
{
    const IOObj* ioobj = wvltfld_->ioobj( true );
    if ( !ioobj )
	return;

    const FilePath fp = ioobj->fullUserExpr();
    FilePath fnm( GetSurveyExportDir(), fp.baseName() );
    fnm.setExtension( "dat" );
    outpfld_->setFileName( fnm.fullPath() );
}


bool uiSeisWvltExp::acceptOK( CallBacker* )
{
    const IOObj* ioobj = wvltfld_->ioobj();
    if ( !ioobj )
	return false;

    const BufferString fnm( outpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( tr("Please enter the output file name") )

    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    if ( !wvlt )
	mErrRet( uiStrings::phrCannotRead( uiStrings::sWavelet()) )
    if ( wvlt->size() < 1 )
	mErrRet( tr("Empty wavelet") )
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenOutpFile() )

    const bool addz = addzfld_->getBoolValue();
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    const StepInterval<float> zpos( wvlt->samplePositions() );
    for ( int idx=0; idx<wvlt->size(); idx++ )
    {
	if ( addz )
	{
	    const float zval = zfac * zpos.atIndex(idx);
	    const od_int64 izval = mRounded( od_int64, 1000 * zval );
	    strm << toString( izval * 0.001 ) << '\t';
	}
	strm << toString(wvlt->samples()[idx]) << '\n';
    }

    if ( !strm.isOK() )
	mErrRet( tr("Possible error during write.") );

    uiString msg = tr("Wavelet successfully exported."
		      "\nDo you want to export more Wavelets?");
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}



// uiSeisWvltCopy
uiSeisWvltCopy::uiSeisWvltCopy( uiParent* p, const IOObj* inioobj )
    : uiDialog(p,Setup(uiStrings::phrCopy(uiStrings::sWavelet()),
		       mNoDlgTitle,mTODOHelpKey))
{
    setOkText( uiStrings::sCopy() );

    wvltinfld_ = new uiWaveletSel( this, true );
    mAttachCB( wvltinfld_->selectionDone, uiSeisWvltCopy::inputChgd );
    if ( inioobj )
	wvltinfld_->setInput( inioobj->key() );

    scalefld_ = new uiGenInput( this, tr("Scale factor for samples"),
				FloatInpSpec(1) );
    scalefld_->attach( alignedBelow, wvltinfld_ );

    wvltoutfld_ = new uiWaveletSel( this, false );
    wvltoutfld_->attach( alignedBelow, scalefld_ );
}


uiSeisWvltCopy::~uiSeisWvltCopy()
{
    detachAllNotifiers();
}


MultiID uiSeisWvltCopy::getMultiID() const
{
    return wvltoutfld_->key();
}


void uiSeisWvltCopy::inputChgd(CallBacker *)
{
    const IOObj* ioobj = wvltinfld_->ioobj( true );
    if ( !ioobj )
	return;

    const BufferString newnm( ioobj->name(), "_copy" );
    wvltoutfld_->setInputText( newnm );
}


bool uiSeisWvltCopy::acceptOK( CallBacker* )
{
    const IOObj* inioobj = wvltinfld_->ioobj();
    const IOObj* outioobj = wvltoutfld_->ioobj();
    if ( !inioobj || !outioobj )
	return false;

    PtrMan<Wavelet> wvlt = Wavelet::get( inioobj );
    if ( !wvlt )
	mErrRet( uiStrings::phrCannotRead( uiStrings::sWavelet()) )

    const float fac = scalefld_->getFValue();
    if ( !mIsUdf(fac) && !mIsZero(fac,mDefEpsF) && !mIsEqual(fac,1.f,mDefEpsF) )
	wvlt->transform( 0.f, fac );

    if ( !wvlt->put(outioobj) )
	mErrRet( tr("Cannot store wavelet on disk") )

    uiString msg = tr("Wavelet successfully copied."
		      "\nDo you want to copy more Wavelets?");
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}

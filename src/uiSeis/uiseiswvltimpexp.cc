/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2006
________________________________________________________________________

-*/


#include "uiseiswvltimpexp.h"

#include "arrayndimpl.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "tabledef.h"
#include "wavelet.h"
#include "waveletio.h"
#include "waveletattrib.h"

#include "uifileinput.h"
#include "uiwaveletsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"

#include <math.h>


uiSeisWvltImp::uiSeisWvltImp( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Wavelet"),mNoDlgTitle,
                                 mODHelpKey(mSeisWvltImpHelpID) ))
    , fd_(*WaveletAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileInput( this, uiStrings::phrInput(uiStrings::phrASCII(
		      uiStrings::sFile())), uiFileInput::Setup()
		      .withexamine(true).examstyle(File::Table) );
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

    wvltfld_ = new uiWaveletIOObjSel( this, false );
    wvltfld_->attach( alignedBelow, scalefld_ );
}


uiSeisWvltImp::~uiSeisWvltImp()
{
    delete &fd_;
}

#define mErrRet(s) { if ( (s).isSet() ) uiMSG().error(s); return false; }



bool uiSeisWvltImp::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("the input file name")) )
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

    if ( !wvltfld_->store(*wvlt,false) )
	mErrRet( tr("Cannot store wavelet on disk") )

    uiString msg = tr("Wavelet successfully imported."
		      "\n\nDo you want to import more Wavelets?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


MultiID uiSeisWvltImp::selKey() const
{
    return wvltfld_->key( true );
}


// uiSeisWvltExp


uiSeisWvltExp::uiSeisWvltExp( uiParent* p )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( uiStrings::sWavelet() ),
				  mNoDlgTitle,
				  mODHelpKey(mSeisWvltImpHelpID) ))
{
    setOkText( uiStrings::sExport() );

    uiWaveletIOObjSel::Setup wvsu; wvsu.compact( false );
    wvltfld_ = new uiWaveletIOObjSel( this, wvsu, true );

    addzfld_ = new uiGenInput( this, uiStrings::phrOutput(SI().zIsTime() ?
				     uiStrings::sTime() : uiStrings::sDepth()),
				     BoolInpSpec(true) );
    addzfld_->attach( alignedBelow, wvltfld_ );

    outpfld_ = new uiFileInput( this, uiStrings::sOutputASCIIFile(),
				uiFileInput::Setup().forread(false) );
    outpfld_->attach( alignedBelow, addzfld_ );
}


bool uiSeisWvltExp::acceptOK( CallBacker* )
{
    ConstRefMan<Wavelet> wvlt = wvltfld_->getWavelet();
    if ( !wvlt )
	return false;
    else if ( wvlt->size() < 1 )
	mErrRet( tr("Empty wavelet") )

    const BufferString fnm( outpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("the output file name")) )
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenOutpFile() )

    const bool addz = addzfld_->getBoolValue();
    const float zfac = mCast( float, SI().zDomain().userFactor() );
    const StepInterval<float> zpos( wvlt->samplePositions() );
    TypeSet<float> samps; wvlt->getSamples( samps );
    for ( int idx=0; idx<wvlt->size(); idx++ )
    {
	if ( addz )
	{
	    const float zval = zfac * zpos.atIndex(idx);
	    const od_int64 izval = mRounded( od_int64, 1000 * zval );
	    strm << toString( izval * 0.001 ) << '\t';
	}
	strm << toString(samps[idx]) << '\n';
    }

    if ( !strm.isOK() )
	mErrRet( tr("Possible error during write.") );

    uiString msg = tr("Wavelet successfully exported."
	              "\nDo you want to export more Wavelets?");
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}

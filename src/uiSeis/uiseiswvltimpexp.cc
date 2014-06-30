/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseiswvltimpexp.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "survinfo.h"
#include "tabledef.h"
#include "wavelet.h"
#include "waveletattrib.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"

#include <math.h>


uiSeisWvltImp::uiSeisWvltImp( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Wavelet"),mNoDlgTitle,
                                 mODHelpKey(mSeisWvltImpHelpID) ))
    , fd_(*WaveletAscIO::getDesc())
    , ctio_(*mMkCtxtIOObj(Wavelet))
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileInput( this, "Input ASCII file", uiFileInput::Setup()
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

    ctio_.ctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, ctio_ );
    wvltfld_->attach( alignedBelow, scalefld_ );
}


uiSeisWvltImp::~uiSeisWvltImp()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}

#define mErrRet(s) { if ( (s).isSet() ) uiMSG().error(s); return false; }



bool uiSeisWvltImp::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( tr("Please enter the input file name") )
    if ( !wvltfld_->commitInput() )
	mErrRet( !wvltfld_->isEmpty() ? uiString::emptyString()
		: tr("Please enter a name for the new wavelet") )
    if ( !dataselfld_->commit() )
	return false;
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open input file") )

    WaveletAscIO aio( fd_ );
    PtrMan<Wavelet> wvlt = aio.get( strm );
    if ( !wvlt )
	mErrRet(aio.errMsg())

    WaveletAttrib wvltattrib( *wvlt );
    const bool quadraturewvlt = mIsEqual(wvltattrib.getAvgPhase(true),90.f,1.f);

    Interval<float> vals;
    wvlt->getExtrValues( vals );
    const float extreme = -1.f*vals.start < vals.stop ? vals.start : vals.stop;
    const int maxsamp = wvlt->getPos( quadraturewvlt ? extreme : 0.f,
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
	msg.append( tr( "Do you want to reposition the center sample?" ) );
	if ( uiMSG().askGoOn(msg) )
	    wvlt->setCenterSample( maxsamp );
    }

    const float fac = scalefld_->getfValue();
    if ( !mIsUdf(fac) && !mIsZero(fac,mDefEpsF) && !mIsEqual(fac,1.f,mDefEpsF) )
	wvlt->transform( 0.f, fac );

    if ( !wvlt->put(ctio_.ioobj) )
	mErrRet( tr("Cannot store wavelet on disk") )

    return true;
}


MultiID uiSeisWvltImp::selKey() const
{
    return ctio_.ioobj ? ctio_.ioobj->key() : MultiID("");
}


// uiSeisWvltExp
uiSeisWvltExp::uiSeisWvltExp( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export Wavelet",mNoDlgTitle,
                                 mODHelpKey(mSeisWvltImpHelpID) ))
{
    setOkText( uiStrings::sExport() );

    wvltfld_ = new uiIOObjSel( this, mIOObjContext(Wavelet) );

    addzfld_ = new uiGenInput( this, BufferString("Output ",
				     SI().zIsTime()?"time":"depth"),
				     BoolInpSpec(true) );
    addzfld_->attach( alignedBelow, wvltfld_ );

    outpfld_ = new uiFileInput( this, "Output ASCII file", uiFileInput::Setup()
				.forread(false) );
    outpfld_->attach( alignedBelow, addzfld_ );
}


bool uiSeisWvltExp::acceptOK( CallBacker* )
{
    const IOObj* ioobj = wvltfld_->ioobj();
    if ( !ioobj ) return false;
    const BufferString fnm( outpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( tr("Please enter the output file name") )

    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    if ( !wvlt )
	mErrRet( tr("Cannot read wavelet") )
    if ( wvlt->size() < 1 )
	mErrRet( tr("Empty wavelet") )
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( tr("Cannot open output file") )

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
	mErrRet( tr("Possible error during write") );

    return true;
}

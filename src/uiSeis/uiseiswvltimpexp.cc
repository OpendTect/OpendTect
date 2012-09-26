/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseiswvltimpexp.h"
#include "wavelet.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "tabledef.h"
#include "strmprov.h"
#include "survinfo.h"

#include "uiioobjsel.h"
#include "uitblimpexpdatasel.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uimsg.h"

#include <math.h>


uiSeisWvltImp::uiSeisWvltImp( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Wavelet import","Import wavelets","103.3.1"))
    , fd_(*WaveletAscIO::getDesc())
    , ctio_(*mMkCtxtIOObj(Wavelet))
{
    inpfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup()
		      .withexamine(true).examstyle(uiFileInput::Setup::Table) );
    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, inpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "103.3.3" );
    dataselfld_->attach( alignedBelow, inpfld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld_ );

    scalefld_ = new uiGenInput( this, "Scale factor for samples",
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

#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }



bool uiSeisWvltImp::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter the input file name" )
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    if ( !wvltfld_->commitInput() )
    {
	sd.close();
	mErrRet( !wvltfld_->isEmpty() ? 0
		: "Please enter a name for the new wavelet" )
    }
    if ( !dataselfld_->commit() )
	{ sd.close(); return false; }

    WaveletAscIO aio( fd_ );
    PtrMan<Wavelet> wvlt = aio.get( *sd.istrm );
    sd.close();
    if ( !wvlt )
	mErrRet(aio.errMsg())

    const int nrsamps = wvlt->size();
    int maxsamp = 0;
    float maxval = fabs( wvlt->samples()[maxsamp] );
    for ( int idx=1; idx<nrsamps; idx++ )
    {
	const float val = fabs( wvlt->samples()[idx] );
	if ( val > maxval )
	    { maxval = val; maxsamp = idx; }
    }
    if ( maxsamp != wvlt->centerSample() )
    {
	BufferString msg( "Highest amplitude is at sample: " );
	msg += maxsamp + 1;
	msg += "\nThe center sample found is: ";
	msg += wvlt->centerSample() + 1;
	msg += "\n\nDo you want to reposition the center sample,"
	       "\nSo it will be at the highest amplitude position?";
	if ( uiMSG().askGoOn( msg ) )
	    wvlt->set( maxsamp, wvlt->sampleRate() );
    }

    const float fac = scalefld_->getfValue();
    if ( fac != 0 && !mIsUdf(fac) && fac != 1 )
    {
	float* samps = wvlt->samples();
	for ( int idx=0; idx<wvlt->size(); idx++ )
	    samps[idx] *= fac;
    }

    if ( !wvlt->put(ctio_.ioobj) )
	mErrRet( "Cannot store wavelet on disk" )

    return true;
}


MultiID uiSeisWvltImp::selKey() const
{
    return ctio_.ioobj ? ctio_.ioobj->key() : MultiID("");
}


uiSeisWvltExp::uiSeisWvltExp( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Wavelet export","Export wavelets","103.3.1"))
{
    wvltfld_ = new uiIOObjSel( this, mIOObjContext(Wavelet) );

    addzfld_ = new uiGenInput( this, BufferString("Put ",
				     SI().zIsTime()?"time":"depth"),
				     BoolInpSpec(true) );
    addzfld_->attach( alignedBelow, wvltfld_ );

    outpfld_ = new uiFileInput( this, "Output file", uiFileInput::Setup()
				.forread(false) );
    outpfld_->attach( alignedBelow, addzfld_ );
}


bool uiSeisWvltExp::acceptOK( CallBacker* )
{
    const IOObj* ioobj = wvltfld_->ioobj();
    if ( !ioobj ) return false;
    const BufferString fnm( outpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter the output file name" )

    PtrMan<Wavelet> wvlt = Wavelet::get( ioobj );
    if ( !wvlt )
	mErrRet( "Cannot read wavelet" )
    if ( wvlt->size() < 1 )
	mErrRet( "Empty wavelet" )
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open output file" )

    const bool addz = addzfld_->getBoolValue();
    const float zfac = SI().zDomain().userFactor();
    const StepInterval<float> zpos( wvlt->samplePositions() );
    for ( int idx=0; idx<wvlt->size(); idx++ )
    {
	if ( addz )
	{
	    const float zval = zfac * zpos.atIndex(idx);
	    const od_int64 izval = mRounded( od_int64, 1000 * zval );
	    *sd.ostrm << toString( izval * 0.001 ) << '\t';
	}
	*sd.ostrm << toString(wvlt->samples()[idx]) << '\n';
    }

    if ( !sd.ostrm->good() )
	{ sd.close(); mErrRet( "Possible error during write" ) }

    sd.close();
    return true;
}

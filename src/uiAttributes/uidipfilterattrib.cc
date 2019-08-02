/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
________________________________________________________________________

-*/

#include "uidipfilterattrib.h"
#include "dipfilterattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribfactory.h"
#include "ioobj.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "survinfo.h"
#include "veldesc.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uifkspectrum.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uislicesel.h"
#include "uispinbox.h"
#include "od_helpids.h"

using namespace Attrib;

const int cMinVal = 3;
const int cMaxVal = 49;
const int cStepVal = 2;

static const char* fltrstrs[] =
{
	"Low",
	"High",
	"Cone",
	0
};

mInitGrpDefAttribUI(uiDipFilterAttrib,DipFilter,
		    tr("Localized Velocity Fan Filter"),sFilterGrp())


uiDipFilterAttrib::uiDipFilterAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mDipFilterAttribHelpID) )

{
    inpfld_ = createInpFld( is2d );

    szfld_ = new uiLabeledSpinBox( this, tr("Filter size") );
    szfld_->box()->setMinValue( cMinVal );
    szfld_->box()->setStep( cStepVal, true );
    szfld_->attach( alignedBelow, inpfld_ );

    uiString fltrlbl = tr("%1 to pass").arg(zIsTime() ? uiStrings::sVelocity()
						      : uiStrings::sDip());
    fltrtpfld_ = new uiGenInput( this, fltrlbl, StringListInpSpec(fltrstrs) );
    fltrtpfld_->valuechanged.notify( mCB(this,uiDipFilterAttrib,filtSel) );
    fltrtpfld_->attach( alignedBelow, szfld_ );

    uiString lbl = tr("Min/max %1").arg( zIsTime()
	    ? uiStrings::sVelocity().withUnit(VelocityDesc::getVelUnit(true))
	    : uiStrings::sDip().withUnit(uiStrings::sDeg()) );
    const char* fldnm = zIsTime() ? " velocity" : " dip";
    velfld_ = new uiGenInput( this, lbl,
		FloatInpSpec().setName( BufferString("Min",fldnm).buf() ),
		FloatInpSpec().setName( BufferString("Max",fldnm).buf() ) );
    velfld_->setElemSzPol( uiObject::Small );
    velfld_->attach( alignedBelow, fltrtpfld_ );

    uiPushButton* dispbut = new uiPushButton( this,
                                             tr("Display F-K panel"), false);
    dispbut->activated.notify( mCB(this,uiDipFilterAttrib,panelbutCB) );
    dispbut->attach( rightTo, velfld_ );

    azifld_ = new uiGenInput( this, tr("Azimuth Filter"),
			      BoolInpSpec(true) );
    azifld_->setValue( false );
    azifld_->attach( alignedBelow, velfld_ );
    azifld_->valuechanged.notify( mCB(this,uiDipFilterAttrib,aziSel) );

    aziintfld_ = new uiGenInput( this, tr("Azimuth to pass (min/max)"),
				FloatInpIntervalSpec()
                                .setName("Min Azimuth",0)
				.setName("Max Azimuth",1) );
    aziintfld_->attach( alignedBelow, azifld_ );

    taperfld_ = new uiGenInput( this, tr("Taper length (%)"),
			       FloatInpSpec().setName("Taper length") );
    taperfld_->attach( alignedBelow, aziintfld_ );

    setHAlignObj( inpfld_ );
    filtSel(0);
    aziSel(0);
}


void uiDipFilterAttrib::panelbutCB( CallBacker* )
{
    PtrMan<uiLinePosSelDlg> dlg = 0;
    if ( is2d_ )
    {
	uiMSG().error( mTODONotImplPhrase() );
	return;
    }
    else
    {
	TrcKeyZSampling cs; inpfld_->getRanges( cs );
	dlg = new uiLinePosSelDlg( this, cs );
    }

    if ( !dlg->go() )
	return;

    const Desc* inpdesc = ads_ ? ads_->getDesc( inpfld_->attribID() ) : 0;
    if ( !inpdesc )
	return;

    const DBKey seisid( inpdesc->getStoredID() );
    PtrMan<IOObj> ioobj = seisid.getIOObj();
    if ( !ioobj )
	return;

    PtrMan<Seis::Provider> prov = Seis::Provider::create( *ioobj );
    if ( !prov )
	return;

    prov->setSelData( new Seis::RangeSelData(dlg->getTrcKeyZSampling()) );
    SeisTrcBuf tbuf( true );
    SeisBufReader bufrdr( *prov, tbuf );
    if ( !bufrdr.execute() )
	return;

    SeisTrcBufArray2D arr2d( &tbuf, false, 0 );
    uiFKSpectrum*  uifk = new uiFKSpectrum( this, true );
    uifk->windowClosed.notify( mCB(this,uiDipFilterAttrib,fkWinCloseCB) );
    uifk->setData( arr2d );
    uifk->show();
}


void uiDipFilterAttrib::fkWinCloseCB( CallBacker* cb )
{
    mDynamicCastGet(uiFKSpectrum*,uifks,cb);
    if ( !uifks ) return;

    float val = uifks->getMinValue();
    if ( !mIsUdf(val) ) velfld_->setValue( val, 0 );
    val = uifks->getMaxValue();
    if ( !mIsUdf(val) ) velfld_->setValue( val, 1 );
}


void uiDipFilterAttrib::filtSel( CallBacker* )
{
    int val = fltrtpfld_->getIntValue();
    bool mode0 = ( val==1 || val==2 );
    bool mode1 = ( !val || val==2 );
    velfld_->setSensitive( mode0, 0, 0 );
    velfld_->setSensitive( mode1, 0, 1 );
}


void uiDipFilterAttrib::aziSel( CallBacker* )
{
    aziintfld_->display( azifld_->getBoolValue() );
}


bool uiDipFilterAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != DipFilter::attribName() )
	return false;

    mIfGetInt( DipFilter::sizeStr(), size, szfld_->box()->setValue(size) )
    mIfGetEnum( DipFilter::typeStr(), type, fltrtpfld_->setValue(type) )
    mIfGetFloat( DipFilter::minvelStr(), minvel,
		 velfld_->setValue(minvel,0) )
    mIfGetFloat( DipFilter::maxvelStr(), maxvel,
		 velfld_->setValue(maxvel,1) )
    mIfGetBool( DipFilter::filteraziStr(), filterazi,
		azifld_->setValue(filterazi) )
    mIfGetFloat( DipFilter::minaziStr(), minazi,
		 aziintfld_->setValue(minazi,0) )
    mIfGetFloat( DipFilter::maxaziStr(), maxazi,
		 aziintfld_->setValue(maxazi,1) )
    mIfGetFloat( DipFilter::taperlenStr(), taperlen,
		 taperfld_->setValue(taperlen) )
    filtSel(0);
    aziSel(0);
    return true;
}


bool uiDipFilterAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiDipFilterAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != DipFilter::attribName() )
	return false;

    mSetInt( DipFilter::sizeStr(), szfld_->box()->getIntValue() );
    mSetEnum( DipFilter::typeStr(), fltrtpfld_->getIntValue() );
    mSetFloat( DipFilter::minvelStr(), velfld_->getFValue(0) );
    mSetFloat( DipFilter::maxvelStr(), velfld_->getFValue(1) );
    mSetBool( DipFilter::filteraziStr(), azifld_->getBoolValue() );
    mSetFloat( DipFilter::minaziStr(), aziintfld_->getFValue(0) );
    mSetFloat( DipFilter::maxaziStr(), aziintfld_->getFValue(1) );
    mSetFloat( DipFilter::taperlenStr(), taperfld_->getFValue() );

    return true;
}


uiRetVal uiDipFilterAttrib::getInput( Desc& desc )
{
    return fillInp( inpfld_, desc, 0 );
}


void uiDipFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( filterszstr(), DipFilter::sizeStr() );

    int val = fltrtpfld_->getIntValue();
    bool mode0 = ( val==1 || val==2 );
    bool mode1 = ( !val || val==2 );
    if ( mode0 )
	params += EvalParam( SI().zIsTime() ? "Minimum velocity" :"Minimum dip",
			     DipFilter::minvelStr() );
    if ( mode1 )
	params += EvalParam( SI().zIsTime() ? "Maximum velocity" :"Maximum dip",
			     DipFilter::maxvelStr() );
}

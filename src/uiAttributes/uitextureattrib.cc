/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
________________________________________________________________________

-*/


#include "uitextureattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribprocessor.h"
#include "binnedvalueset.h"
#include "trckeyzsampling.h"
#include "dataclipper.h"
#include "ioobj.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "survinfo.h"
#include "textureattrib.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiselsurvranges.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

using namespace Attrib;

static const char* actionstr[] =
{
    "Contrast",
    "Dissimilarity",
    "Homogeneity",
    "Angular Second Moment",
    "Energy",
    "Entropy",
    "GLCM Mean",
    "GLCM Variance",
    "GLCM Standard Deviation",
    "GLCM Correlation",
    0
};

mInitAttribUINoSynth( uiTextureAttrib, Texture,
		      uiStrings::sTexture(), sBasicGrp() );


uiTextureAttrib::uiTextureAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mTextureAttribHelpID) )
{
    inpfld_ = createInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(),
		FloatInpIntervalSpec().setName("Z start",0)
				      .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );

    steerfld_ = new uiSteeringSel( this, 0, is2d, false );
    steerfld_->attach( alignedBelow, gatefld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d );
    stepoutfld_->setFieldNames( "Stepout Inl", "Stepout Crl" );
    stepoutfld_->attach( alignedBelow, steerfld_ );

    actionfld_ = new uiGenInput( this, uiStrings::sOutput(),
		    StringListInpSpec(actionstr) );
    actionfld_->attach( alignedBelow, stepoutfld_ );

    glcmsizefld_ = new uiGenInput( this, tr("GLCM size"),
                                   BoolInpSpec(true,toUiString("16x16"),
				   toUiString("32x32")) );
    glcmsizefld_->attach( alignedBelow, actionfld_ );

    globalminfld_ = new uiGenInput( this, uiStrings::phrInput(
				    uiStrings::sDataRange()), FloatInpSpec() );
    globalminfld_->setElemSzPol(uiObject::Small);
    globalminfld_->attach( alignedBelow, glcmsizefld_ );
    globalmaxfld_ = new uiGenInput( this, uiString::empty(),
					  FloatInpSpec() );
    globalmaxfld_->setElemSzPol(uiObject::Small);
    globalmaxfld_->attach( rightOf, globalminfld_ );

    uiPushButton* analysebut = new uiPushButton( this, uiStrings::sCalculate(),
				 mCB(this,uiTextureAttrib,analyseCB), false );
    analysebut->attach( rightOf, globalmaxfld_ );
    setHAlignObj( inpfld_ );
}


bool uiTextureAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName()!=Texture::attribName() )
	return false;

    mIfGetFloatInterval( Texture::gateStr(),
			gate, gatefld_->setValue(gate) );
    mIfGetFloat( Texture::globalminStr(), globalmin,
		globalminfld_->setValue(globalmin) );
    mIfGetFloat( Texture::globalmaxStr(), globalmax,
		globalmaxfld_->setValue(globalmax) );
    mIfGetBinID( Texture::stepoutStr(), stepout,
		stepoutfld_->setBinID(stepout) );
    mIfGetInt( Texture::glcmsizeStr(), glcmsize,
	       glcmsizefld_->setValue(glcmsize==16) );

    return true;
}

bool uiTextureAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiTextureAttrib::setOutput( const Desc& desc )
{
    actionfld_->setValue( desc.selectedOutput() );
    return true;
}


bool uiTextureAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName()!=Texture::attribName() )
	return false;

    const float globalmin = globalminfld_->getFValue();
    const float globalmax = globalmaxfld_->getFValue();
    if ( mIsEqual( globalmin, globalmax, 1e-3 ))
    {
	uiString errstr =
	    tr("Minimum and Maximum values cannot be the same.\n"
	       "Values represent the clipping range of the input.");
	uiMSG().error(errstr);
	return false;
    }

    mSetFloatInterval( Texture::gateStr(), gatefld_->getFInterval() );
    mSetFloat( Texture::globalminStr(), globalmin );
    mSetFloat( Texture::globalmaxStr(), globalmax );

    BinID stepout( stepoutfld_->getBinID() );
    mSetBinID( Texture::stepoutStr(), stepout );
    const bool dosteer = steerfld_->willSteer();
    mSetBool( Texture::steeringStr(), dosteer );
    mSetInt( Texture::glcmsizeStr(), glcmsizefld_->getBoolValue() ? 16 : 32 );

    return true;
}


uiRetVal uiTextureAttrib::getInput( Desc& desc )
{
    uiRetVal uirv = fillInp( inpfld_, desc, 0 );
    uirv.add( fillInp( steerfld_, desc, 1 ) );
    return uirv;
}


bool uiTextureAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, actionfld_->getIntValue() );
    return true;
}


void uiTextureAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Texture::gateStr() );
    params += EvalParam( stepoutstr(), Texture::stepoutStr() );

    EvalParam ep( "Output" ); ep.evaloutput_ = true;
    params += ep;
}


class uiSubSelForAnalysis : public uiDialog
{ mODTextTranslationClass(uiSubSelForAnalysis);
public:
uiSubSelForAnalysis( uiParent* p,const DBKey& mid, bool is2d )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(tr("data for analysis")),
				 mNoDlgTitle,mNoHelpKey))
    , linesfld_(0)
    , subvolfld_(0)
{
    nrtrcfld_ = new uiGenInput( this, tr("Nr of Traces for Examination"),
				IntInpSpec(50) );

    if ( is2d )
    {
	linesfld_ = new uiSeis2DLineNameSel( this, true );
	linesfld_->setDataSet( mid );
	linesfld_->attach( alignedBelow, nrtrcfld_ );
    }
    else
    {
	subvolfld_ = new uiSelSubvol( this, false );
	subvolfld_->attach( alignedBelow, nrtrcfld_ );
    }
}

int nrTrcs()
{ return nrtrcfld_->getIntValue(); }

Pos::GeomID geomID() const
{ return linesfld_ ? linesfld_->getInputGeomID() : mUdfGeomID; }

bool acceptOK()
{
    if ( nrtrcfld_->getIntValue()< 1 )
    {
	uiMSG().error(uiStrings::phrPlsSelectAtLeastOne(uiStrings::sTrace()));
	return false;
    }

    return true;
}

TrcKeyZSampling subVol() const
{
    TrcKeyZSampling cs;
    if ( subvolfld_ )
	cs = subvolfld_->getSampling();
    return cs;
}


protected:

    uiGenInput*			nrtrcfld_;
    uiSelSubvol*		subvolfld_;
    uiSeis2DLineNameSel*	linesfld_;
};


void uiTextureAttrib::analyseCB( CallBacker* )
{
    const Attrib::Desc* inpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !inpdesc )
	return;

    PtrMan<IOObj> ioobj = DBKey(inpdesc->getStoredID(true)).getIOObj();
    if ( !ioobj )
	{ uiMSG().error( tr("Select a valid input") ); return; }

    uiSubSelForAnalysis subseldlg( this, ioobj->key(), inpdesc->is2D() );
    if ( !subseldlg.go() )
	return;

    SeisIOObjInfo seisinfo( ioobj );
    TrcKeyZSampling cs;
    if ( inpdesc->is2D() )
    {
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	const Pos::GeomID geomid = subseldlg.geomID();
	seisinfo.getRanges( geomid, trcrg, zrg );
	cs.hsamp_.setCrlRange( trcrg );
	const auto lnr = geomid.lineNr();
	cs.hsamp_.setInlRange( StepInterval<int>(lnr,lnr,1) );
	cs.zsamp_ = zrg;
    }
    else
    {
	cs = subseldlg.subVol();
	seisinfo.getRanges( cs );
    }

    const int nrtrcs = subseldlg.nrTrcs();
    SeisTrcBuf buf( true );
    if ( readInpAttrib(buf,cs,nrtrcs) )
	calcAndSetMinMaxVal( buf );
}


bool uiTextureAttrib::readInpAttrib( SeisTrcBuf& buf, const TrcKeyZSampling& cs,
				     int nrtrcs ) const
{
    const Attrib::Desc* inpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !inpdesc )
	return false;

    PtrMan<Attrib::DescSet> descset = ads_->optimizeClone(inpfld_->attribID());
    if ( !descset )
	return false;

    PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan;
    SelSpec sp( 0 );
    sp.set( *inpdesc );
    aem->setAttribSet( descset );
    aem->setAttribSpec( sp );
    if ( inpdesc->is2D() )
	aem->setGeomID( Pos::GeomID(cs.hsamp_.start_.inl()) );

    aem->setSubSel( Survey::FullSubSel(cs) );
    TypeSet<TrcKey> trckeys;
    cs.hsamp_.getRandomSet( nrtrcs, trckeys );
    BinnedValueSet bidvals( 0, false );
    for ( int idx=0; idx<trckeys.size(); idx++ )
	bidvals.add( trckeys[idx].binID() );

    uiRetVal uirv;
    Interval<float> zrg( cs.zsamp_ );
    PtrMan<Processor> proc =
	aem->createTrcSelOutput( uirv, bidvals, buf, 0, &zrg );

    if ( !proc )
	{ uiMSG().error( uirv ); return false; }

    uiTaskRunner dlg( this );
    return dlg.execute( *proc );
}


static void checkAndSetSymmetric( Interval<float>& range )
{
    if ( range.start>= 0 || range.stop<= 0 )
	return;

    const float leftarm = 0 - range.start;
    const float rightarm = 0 + range.stop;
    if ( mIsZero(leftarm,1e-6) || mIsZero(rightarm,1e-6) )
	return;

    const float ratio = ( leftarm<rightarm ? leftarm/rightarm
					   : rightarm/leftarm );
    if ( ratio<0.8 )
	return;

    if ( leftarm<rightarm )
	range.start = 0-rightarm;
    else
	range.stop = leftarm;
}


void uiTextureAttrib::calcAndSetMinMaxVal( const SeisTrcBuf& bufs )
{
    const SeisTrc* seisttrc = bufs.get( 0 );
    TypeSet<float> vals;
    vals.setCapacity( bufs.size() * bufs.get(0)->size(), false );

    for ( int trcnr=0; trcnr<bufs.size(); trcnr++ )
    {
	seisttrc = bufs.get( trcnr );
	const int nrsamples = seisttrc->size();
	for ( int sampnr=0; sampnr<nrsamples; sampnr++ )
	{
	    float val = seisttrc->get( sampnr, 0 );
	    if ( !mIsUdf(val) )
		vals += val;
	}
    }

    DataClipper dc;
    Interval<float> range;
    dc.calculateRange( vals.arr(), vals.size(), 0.01, 0.01, range );
    checkAndSetSymmetric( range );
    globalminfld_->setValue( range.start );
    globalmaxfld_->setValue( range.stop );
}

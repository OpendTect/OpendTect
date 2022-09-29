/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
#include "binidvalset.h"
#include "trckeyzsampling.h"
#include "dataclipper.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "survinfo.h"
#include "textureattrib.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uimsg.h"
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

mInitAttribUI(uiTextureAttrib,Texture,"Texture",sKeyBasicGrp())

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
			       mJoinUiStrs(sData(),sRange())), FloatInpSpec() );
    globalminfld_->setElemSzPol(uiObject::Small);
    globalminfld_->attach( alignedBelow, glcmsizefld_ );
    globalmaxfld_ = new uiGenInput( this, uiStrings::sEmptyString(),
							      FloatInpSpec() );
    globalmaxfld_->setElemSzPol(uiObject::Small);
    globalmaxfld_->attach( rightOf, globalminfld_ );

    uiPushButton* analysebut = new uiPushButton( this, tr("Compute"),
				 mCB(this,uiTextureAttrib,analyseCB), false );
    analysebut->attach( rightOf, globalmaxfld_ );
    setHAlignObj( inpfld_ );
}


uiTextureAttrib::~uiTextureAttrib()
{}


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


bool uiTextureAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
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
uiSubSelForAnalysis( uiParent* p,const MultiID& mid, bool is2d,const char* anm )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrSelect(tr("data for analysis")),
				 mNoDlgTitle,mNoHelpKey))
    , attribnm_(anm)
    , linesfld_(0)
    , subvolfld_(0)
{
    nrtrcfld_ = new uiGenInput( this, tr("Nr of Traces for Examination"),
				IntInpSpec(50) );

    if ( is2d )
    {
	SeisIOObjInfo objinfo( mid );
	BufferStringSet linenames;
	objinfo.getLineNames( linenames );
	linesfld_ = new uiLabeledComboBox( this, tr("Analysis on line:") );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( toUiString(linenames.get(idx)) );

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

LineKey lineKey() const
{ return LineKey( linesfld_ ? linesfld_->box()->text() : "", attribnm_ ); }

bool acceptOK(CallBacker*) override
{
    if ( nrtrcfld_->getIntValue()< 1 )
    {
	uiMSG().error(tr("Select at least one trace"));
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

    BufferString	attribnm_;

    uiGenInput*		nrtrcfld_;
    uiSelSubvol*	subvolfld_;
    uiLabeledComboBox*	linesfld_;
};


void uiTextureAttrib::analyseCB( CallBacker* )
{
    const Attrib::Desc* inpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !inpdesc )
	return;

    LineKey lk( inpdesc->getStoredID(true) );
    PtrMan<IOObj> ioobj = IOM().get( MultiID(lk.lineName().buf()) );
    if ( !ioobj )
    {
	uiMSG().error( tr("Select a valid input") );
	return;
    }

    uiSubSelForAnalysis subseldlg( this, ioobj->key(), inpdesc->is2D(),
				   lk.attrName() );
    if ( !subseldlg.go() )
	return;

    SeisIOObjInfo seisinfo( ioobj );
    TrcKeyZSampling cs;
    if ( inpdesc->is2D() )
    {
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	seisinfo.getRanges(
		Survey::GM().getGeomID(subseldlg.lineKey().lineName()),
		trcrg, zrg );
	cs.hsamp_.setCrlRange( trcrg );
	cs.hsamp_.setInlRange( Interval<int>(0,0) );
	cs.zsamp_ = zrg;
	lk = subseldlg.lineKey();
    }
    else
    {
	cs = subseldlg.subVol();
	seisinfo.getRanges( cs );
    }

    const int nrtrcs = subseldlg.nrTrcs();
    SeisTrcBuf buf( true );
    if ( readInpAttrib(buf,cs,nrtrcs,lk) )
	calcAndSetMinMaxVal( buf );
}


bool uiTextureAttrib::readInpAttrib( SeisTrcBuf& buf, const TrcKeyZSampling& cs,
				      int nrtrcs, const LineKey& lk) const
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
	aem->setGeomID( Survey::GM().getGeomID(lk.lineName().buf()) );

    aem->setTrcKeyZSampling( cs );
    TypeSet<TrcKey> trckeys;
    cs.hsamp_.getRandomSet( nrtrcs, trckeys );
    BinIDValueSet bidvals( 0, false );
    for ( int idx=0; idx<trckeys.size(); idx++ )
	bidvals.add( trckeys[idx].position() );

    uiString errmsg;
    Interval<float> zrg( cs.zsamp_ );
    PtrMan<Processor> proc =
	aem->createTrcSelOutput( errmsg, bidvals, buf, 0, &zrg );

    if ( !proc )
    {
	uiMSG().error( errmsg );
	return false;
    }

    uiTaskRunner dlg( const_cast<uiTextureAttrib*>(this) );
    return TaskRunner::execute( &dlg, *proc );
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
    if ( !bufs.size() )
	return;

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

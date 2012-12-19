/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uitextureattrib.cc 27530 2012-11-19 09:49:13Z kristofer.tingdahl@dgbes.com $";

#include "uitextureattrib.h"
#include "textureattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"
#include "seistrc.h"

#include "attribengman.h"
#include "attribdescset.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "binidvalset.h"
#include "cubesampling.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "volstatsattrib.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uiselsurvranges.h"
#include "uitable.h"
#include "uitaskrunner.h"

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
    "CLCM Correlation",
    0
};

mInitAttribUI(uiTextureAttrib,Texture,"Texture",sKeyBasicGrp())

uiTextureAttrib::uiTextureAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
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

    actionfld_ = new uiGenInput( this, "Output", 
		    StringListInpSpec(actionstr) );
    actionfld_->attach( alignedBelow, stepoutfld_ );
    glcmsizefld_ = new uiGenInput( this, "GLCM size",
		    BoolInpSpec(true,"16x16","32x32") );
    glcmsizefld_->attach( alignedBelow, actionfld_ );
    globalminfld_ = new uiGenInput( this, "Input Data Range", FloatInpSpec() );
    globalminfld_->setElemSzPol(uiObject::Small);
    globalminfld_->attach( alignedBelow, glcmsizefld_ );
    globalmaxfld_ = new uiGenInput( this, "",
		    FloatInpSpec() );
    globalmaxfld_->setElemSzPol(uiObject::Small);
    globalmaxfld_->attach( rightOf, globalminfld_ );

    analysebut_ = new uiPushButton( this, "Compute",
				 mCB(this, uiTextureAttrib, analyseCB), false );
    analysebut_->attach( rightOf, globalmaxfld_ );
    setHAlignObj( inpfld_ );
}


bool uiTextureAttrib::setParameters( const Desc& desc )
{
    BufferString attribname = desc.attribName();
    if ( attribname != Texture::attribName() )
	return false;

    mIfGetFloatInterval( Texture::gateStr(), 
			gate, gatefld_->setValue(gate) );
    mIfGetFloat( Texture::globalminStr(), globalmin,
		globalminfld_->setValue(globalmin) );
    mIfGetFloat( Texture::globalmaxStr(), globalmax,
		globalmaxfld_->setValue(globalmax) );
    mIfGetEnum( Texture::actionStr(), action,
		actionfld_->setValue(action) );
    mIfGetBinID( Texture::stepoutStr(), stepout,
		stepoutfld_->setBinID(stepout) );
    mIfGetBool( Texture::glcmsizeStr(), glcmsize,
		glcmsizefld_->setValue(glcmsize) );

    return true;
}

bool uiTextureAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );
    return true;
}


bool uiTextureAttrib::getParameters( Desc& desc )
{
    BufferString attribname = desc.attribName();
    if ( attribname != Texture::attribName() )
	return false;
    
    const float globalmin = globalminfld_->getfValue();
    const float globalmax = globalmaxfld_->getfValue();
    if ( mIsEqual( globalmin, globalmax, 1e-3 ))
    {
	BufferString errstr = 
	    "Minimum and Maximum values cannot be the same.\n";
	errstr += "Values represent the clipping range of the input.";
	uiMSG().error( errstr.buf() );
	return false;
    }

    mSetFloatInterval( Texture::gateStr(), gatefld_->getFInterval() );
    mSetFloat( Texture::globalminStr(), globalmin );
    mSetFloat( Texture::globalmaxStr(), globalmax );
  
    bool dosteer = false;
    mSetEnum( Texture::actionStr(), actionfld_->getIntValue() );
    BinID stepout( stepoutfld_->getBinID() );
    mSetBinID( Texture::stepoutStr(), stepout );
    dosteer = steerfld_->willSteer();
    mSetBool( Texture::steeringStr(), dosteer );
    mSetBool( Texture::glcmsizeStr(), glcmsizefld_->getBoolValue() );

    return true;
}

bool uiTextureAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );
    return true;
}

void uiTextureAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Texture::gateStr() );
    params += EvalParam( stepoutstr(), Texture::stepoutStr() );
}


class uiSubSelForAnalysis : public uiDialog
{
public:
uiSubSelForAnalysis( uiParent* p,const MultiID& mid, bool is2d,const char* anm )
    : uiDialog(p,uiDialog::Setup("Select data","For analysis",mNoHelpID)) 
    , attribnm_(anm)
    , linesfld_(0)
    , subvolfld_(0)
{
    nrtrcfld_ = new uiGenInput( this, "Nr of Traces for Examination",
	    			IntInpSpec(50) );
    
    if ( is2d )
    {
	SeisIOObjInfo objinfo( mid );
	BufferStringSet linenames;
	objinfo.getLineNamesWithAttrib( attribnm_, linenames );
	linesfld_ = new uiLabeledComboBox( this, "Analyisis on line:" );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );
	
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
{ return LineKey( linesfld_->box()->text(), attribnm_ ); }

CubeSampling subVol() const
{
    CubeSampling cs;
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
    Attrib::Desc* inpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !inpdesc )
	return;

    LineKey lk( inpdesc->getStoredID(true) );
    PtrMan<IOObj> ioobj = IOM().get( MultiID(lk.lineName()) );

    if ( !ioobj )
	return uiMSG().error( "Select a valid input" );

    SeisIOObjInfo seisinfo( ioobj );
    CubeSampling cs;
   
    uiSubSelForAnalysis subseldlg( this, ioobj->key(), seisinfo.is2D(),
	    			   lk.attrName() );
    subseldlg.go();

    if ( seisinfo.is2D() )
    {
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	seisinfo.getRanges( subseldlg.lineKey(), trcrg, zrg );
	cs.hrg.setCrlRange( trcrg );
	cs.hrg.setInlRange( Interval<int>(0,0) );
	cs.zrg = zrg;
	lk = subseldlg.lineKey();
    }
    else
    {
	cs = subseldlg.subVol();
	seisinfo.getRanges( cs );
    }

    const int nrtrcs = subseldlg.nrTrcs();
    if ( nrtrcs <= 0 )
	return uiMSG().error( "Select proper number of traces" );

    readSampAttrib( cs, nrtrcs, lk );
}


void uiTextureAttrib::readSampAttrib(CubeSampling& cs, int nrtrcs, LineKey& lk)
    {
    Attrib::Desc* inpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !inpdesc )
	return;

    PtrMan<Attrib::DescSet> descset = ads_->optimizeClone(inpfld_->attribID());
    if ( !descset )
	return;
   
    Attrib::DescID attribid = descset->addDesc(inpdesc );

    PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan;

    TypeSet<SelSpec> attribspecs;
    SelSpec sp( 0, attribid );
    sp.set( *inpdesc );
    attribspecs += sp;

    aem->setAttribSet( descset );
    aem->setAttribSpecs( attribspecs );
    if ( inpdesc->is2D() )
	aem->setLineKey( lk );
    aem->setCubeSampling( cs );

    TypeSet<BinID> bidset;
    cs.hrg.getRandomSet( nrtrcs, bidset );

    BinIDValueSet bidvals( 0, false );
    for ( int idx=0; idx<bidset.size(); idx++ )
	bidvals.add( bidset[idx] );

    BufferString errmsg;
    SeisTrcBuf bufs( true );
    Interval<float> zrg( cs.zrg );
    PtrMan<Processor> proc =
	aem->createTrcSelOutput( errmsg, bidvals, bufs, 0, &zrg );

    if ( !proc )
    {
	uiMSG().error( errmsg );
	return;
    }

    uiTaskRunner dlg( this );
    if ( !dlg.execute(*proc) )
	return;

    setMinMaxVal( bufs );
}


void uiTextureAttrib::setMinMaxVal( const SeisTrcBuf& bufs )
{
    const SeisTrc* seisttrc = bufs.get( 0 );
    const int nrsamples = bufs.get(0)->size();
    float minval = seisttrc->get( 0, 0 );
    float maxval = minval;

    for ( int trcnr=0; trcnr<bufs.size(); trcnr++ )
    {
	seisttrc = bufs.get( trcnr );
	for ( int sampnr=0; sampnr<nrsamples; sampnr++ )
	{
	    float val = seisttrc->get( sampnr, 0 );
	    if ( !mIsUdf(val) )
	    {
		if ( val<minval )
		    minval=val;
		else if ( val>maxval )
		    maxval=val;
	    }
	}
    }

    globalminfld_->setValue(minval);
    globalmaxfld_->setValue(maxval);
}



/*+
 * (C) JOANNEUM RESEARCH; http://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz;
 * DATE     : November 2013
-*/


#include "uiGLCM_attrib.h"
#include "GLCM_attrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

#include "trckeyzsampling.h"
#include "linekey.h"
#include "seisbuf.h"
#include "seistrc.h"

#include "uimsg.h"
#include "uibutton.h"
#include "bufstring.h"
#include "uicombobox.h"
#include "seisioobjinfo.h"
#include "ioobj.h"
#include "uiselsurvranges.h"
#include "uitaskrunner.h"
#include "attribprocessor.h"
#include "binidvalset.h"
#include "dataclipper.h"
#include "ioman.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribfactory.h"


using namespace Attrib;

static const char* attribstr[] =
{
	"Energy f1",
	"Contrast f2",
	"Correlation f3",
	"Variance f4",
	"Inverse Difference Moment f5",
	"Sum Average f6",
	"Sum Variance f7",
	"Sum Entropy f8",
	"Entropy f9",
	"Difference Variance f10",
	"Difference Entropy f11",
	"Information Measures of Correlation f12",
	"Information Measures of Correlation f13",
	"Homogeneity g1",
	"Sum Mean g2",
	"Maximum Probability g3",
	"Cluster Tendency g4",
	"Cluster Shade g5",
	"Cluster Prominence g6",
	"Dissimilarity g7",
	"Difference Mean g8",
	"Autocorrelation g9",
	"Inertia g10",
	0
};

static const char* directionstr[]=
{
	"Azimuth 0, Dip 0",
	"Azimuth 0, Dip 45",
	"Azimuth 0, Dip 90",
	"Azimuth 0, Dip 135",
	"Azimuth 45, Dip 0",
	"Azimuth 45, Dip 45",
	"Azimuth 45, Dip 135",
	"Azimuth 90, Dip 0",
	"Azimuth 90, Dip 45",
	"Azimuth 90, Dip 135",
	"Azimuth 135, Dip 0",
	"Azimuth 135, Dip 45",
	"Azimuth 135, Dip 135",
	"Inline Direction",
	"Crossline Direction",
	"Timeslice/Depthslice Direction",
	"All Directions",
	0
};


mInitAttribUI( uiGLCM_attrib, GLCM_attrib, "Texture - Directional",
	       sKeyBasicGrp() )

uiGLCM_attrib::uiGLCM_attrib( uiParent* p, bool is2d )
    : uiAttrDescEd( p, is2d, mODHelpKey(mTextureDirectionalHelpID) )
{
    inpfld_ = createInpFld( is2d );

    limitfld_ = new uiGenInput( this, tr("Min / Max of Input Data"),
			       FloatInpSpec().setName("Min amplitude"),
			       FloatInpSpec().setName("Max amplitude"));
    limitfld_->setElemSzPol( uiObject::Small );
    limitfld_->attach( alignedBelow, inpfld_);

    uiPushButton* analyseButton = new uiPushButton(this,
				tr("Compute Amplitude Range"),
				mCB(this, uiGLCM_attrib, analyseData), false);
    analyseButton->attach( rightOf, limitfld_);

    numbergreyfld_ = new uiGenInput( this, tr("Number of Grey Levels"),
				     IntInpSpec() );
    numbergreyfld_->attach( alignedBelow, limitfld_ );

    stepoutfld_ = new uiStepOutSel( this, is2d, tr("Number of Traces") );
    stepoutfld_->setFieldNames( "Number of Traces (Inl)",
				"Number of Traces (Crl)" );
    const StepInterval<int> intv( 0, 10, 1 );
    stepoutfld_->setInterval( intv, intv );
    stepoutfld_->attach( alignedBelow,numbergreyfld_ );

    samprangefld_ = new uiGenInput( this, tr("Vertical Search Window (+/-)"),
				    IntInpSpec() );
    samprangefld_->attach( alignedBelow, stepoutfld_ );

    attributefld_ = new uiGenInput( this, tr("GLCM Attribute"),
				    StringListInpSpec(attribstr) );
    attributefld_ ->valuechanged.notify( mCB( this, uiGLCM_attrib,
					 GLCMattributeSel) );
    attributefld_ ->attach( alignedBelow, samprangefld_ );

    directfld_ = new uiGenInput( this, tr("Direction of Calculation"),
				 StringListInpSpec(directionstr) );
    directfld_ ->valuechanged.notify( mCB( this, uiGLCM_attrib,
				      GLCMdirectionSel) );
    directfld_ ->attach( alignedBelow, attributefld_ );


    steerfld_ = new uiSteeringSel( this, 0 , is2d );
    steerfld_->steertypeSelected_.notify( mCB( this, uiGLCM_attrib,
					       steerTypeSel ) );
    steerfld_->attach( alignedBelow, directfld_ );

    GLCMattributeSel(0);

    GLCMdirectionSel(0);

    setHAlignObj( inpfld_ );
}

void uiGLCM_attrib::GLCMattributeSel( CallBacker* )
{
//	const int attribval = attributefld_->getIntValue();
}

void uiGLCM_attrib::GLCMdirectionSel( CallBacker* )
{
//	const int directionval = directfld_->getIntValue();
}

bool uiGLCM_attrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != GLCM_attrib::attribName() )
	return false;

    mIfGetInt( GLCM_attrib::numbergreyStr(), greylevels,
	      numbergreyfld_->setValue(greylevels) );
    mIfGetFloat( GLCM_attrib::minlimitStr(), minlimit,
		limitfld_->setValue(minlimit,0) );
    mIfGetFloat( GLCM_attrib::maxlimitStr(), maxlimit,
		limitfld_->setValue(maxlimit,1) );
    mIfGetBinID( GLCM_attrib::stepoutStr(), stepout,
		stepoutfld_->setBinID(stepout) );
    mIfGetEnum( GLCM_attrib::attributeStr(), attribute,
	       attributefld_->setValue(attribute) );
    mIfGetEnum( GLCM_attrib::directionStr(), direction,
	       directfld_ ->setValue(direction) );
    mIfGetInt( GLCM_attrib::sampStr(), samprange,
	      samprangefld_->setValue(samprange) );


    GLCMattributeSel(0);
    GLCMdirectionSel(0);

    return true;
}

bool uiGLCM_attrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != GLCM_attrib::attribName() )
	return false;

    const float minlimit=limitfld_->getFValue(0);
    const float maxlimit=limitfld_->getFValue(1);
    if ( mIsEqual( minlimit, maxlimit, 1e-3 ))
    {
	const uiString errorLimit =
		    tr("Minimum and Maximum values cannot be the same. "
		       "Values represent the clipping range of the input.");
	uiMSG().error( errorLimit );
	return false;
    }

    mSetInt( GLCM_attrib::sampStr(), samprangefld_->getIntValue() );
    mSetFloat( GLCM_attrib::minlimitStr(), limitfld_->getFValue(0) );
    mSetFloat( GLCM_attrib::maxlimitStr(), limitfld_->getFValue(1) );

    mSetInt( GLCM_attrib::numbergreyStr(), numbergreyfld_->getIntValue() );

    const char* attribut = attributefld_->text();
    const char* direction = directfld_->text();
    BufferStringSet strattribut(attribstr);
    BufferStringSet strdirect(directionstr);

    mSetEnum( GLCM_attrib::attributeStr(), strattribut.indexOf(attribut) );
    mSetEnum( GLCM_attrib::directionStr(), strdirect.indexOf(direction) );
    mSetBinID( GLCM_attrib::stepoutStr(), stepoutfld_->getBinID() );
    mSetBool( GLCM_attrib::steeringStr(), steerfld_->willSteer() );

    return true;
}

bool uiGLCM_attrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    putInp( steerfld_, desc, 1 );

    return true;
}

bool uiGLCM_attrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    fillInp( steerfld_, desc, 1 );

    return true;
}

void uiGLCM_attrib::steerTypeSel( CallBacker* )
{
    if ( is2D() && steerfld_->willSteer() && !inpfld_->isEmpty() )
    {
	const char* steertxt = steerfld_->text();
	if ( steertxt )
	{
	    LineKey inp( inpfld_->getInput() );
	    LineKey steer( steertxt );
	    if ( inp.lineName() != steer.lineName() )
		    steerfld_->clearInpField();
	}
    }
}


void uiGLCM_attrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( zGateLabel(), GLCM_attrib::sampStr() );
    params += EvalParam( stepoutstr(), GLCM_attrib::stepoutStr() );

    EvalParam ep( "GLCM Attribute");
    ep.evaloutput_=true;
    params += ep;
}


class uiSubSelForAnalysis : public uiDialog
{ mODTextTranslationClass(uiSubSelForAnalysis);
public:
uiSubSelForAnalysis( uiParent* p,const MultiID& mid, bool is2d,const char* anm )
    : uiDialog(p,uiDialog::Setup(tr("Select data"),tr("For analysis"),
				 mNoHelpKey))
    , attribnm_(anm)
    , linesfld_(0)
    , subvolfld_(0)
{
    nrtrcfld_ = new uiGenInput( this, tr("Nr of Traces for Examination"),
				IntInpSpec(100) );

    if ( is2d )
    {
	SeisIOObjInfo objinfo( mid );
	BufferStringSet linenames;
	objinfo.getLineNames( linenames );
	linesfld_ = new uiLabeledComboBox( this, tr("Analyisis on line") );
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
{ return nrtrcfld_->getIntValue();
}

LineKey lineKey() const
{ return LineKey( linesfld_ ? linesfld_->box()->text() : "", attribnm_ ); }

bool acceptOK(CallBacker*)
{
    if ( nrtrcfld_->getIntValue()< 1 )
    {
	uiMSG().error( tr("Select at least one trace") );
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


void uiGLCM_attrib::analyseData( CallBacker* )
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
    if ( readInputCube(buf,cs,nrtrcs,lk) )
	determineMinMax( buf );
}


bool uiGLCM_attrib::readInputCube( SeisTrcBuf& buf, const TrcKeyZSampling& cs,
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

    uiTaskRunner dlg( const_cast<uiGLCM_attrib*>(this) );
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


void uiGLCM_attrib::determineMinMax( const SeisTrcBuf& bufs )
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
	limitfld_->setValues( range.start, range.stop);
}

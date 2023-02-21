/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprobdenfuncgen.h"

#include "uichecklist.h"
#include "uieditpdf.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimnemonicsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uiunitsel.h"

#include "gaussianprobdenfunc.h"
#include "od_helpids.h"
#include "probdenfunctr.h"
#include "sampledprobdenfunc.h"
#include "statruncalc.h"
#include "unitofmeasure.h"


static const float cMaxSampledBinVal = 100.0f;


static bool writePDF( const ProbDenFunc& pdf, const IOObj& ioobj )
{
    uiString emsg;
    if ( ProbDenFuncTranslator::write(pdf,ioobj,&emsg) )
	return true;

    uiString msg = emsg;
    if ( !msg.isSet() )
	msg = uiStrings::phrCannotWrite(od_static_tr("writePDF","PDF to disk"));
    uiMSG().error( msg );
    return false;
}


class uiProbDenFuncGenSampled : public uiDialog
{ mODTextTranslationClass(uiProbDenFuncGenSampled);
public:

			uiProbDenFuncGenSampled(uiParent*,int nrdim,bool gauss,
						MultiID&,
						const MnemonicSelection&,
						const BufferStringSet& varnms);
			~uiProbDenFuncGenSampled();

    inline bool		isGauss() const { return !expstdflds_.isEmpty(); }
    bool		getFromScreen();
    ProbDenFunc*	getPDF() const;

private:

    uiSpinBox*		nrbinsfld_;
    ObjectSet<uiGenInput> nmflds_;
    ObjectSet<uiGenInput> rgflds_;
    ObjectSet<uiUnitSel>  unitflds_;
    ObjectSet<uiGenInput> expstdflds_;
    ObjectSet<uiGenInput> ccflds_;
    uiIOObjSel*		outfld_;

    MultiID&		ioobjky_;
    const int		nrdims_;
    int			nrbins_;
    TypeSet<Interval<float> > rgs_;
    TypeSet<float>	exps_;
    TypeSet<float>	stds_;
    TypeSet<float>	ccs_;
    BufferStringSet	dimnms_;

    bool		acceptOK(CallBacker*) override;
    void		initDlg(CallBacker*);
    void		unitChgCB(CallBacker*);
    void		chgCB(CallBacker*);
    void		rgChg(CallBacker*);
};


class uiProbDenFuncGenGaussian : public uiDialog
{ mODTextTranslationClass(uiProbDenFuncGenGaussian);
public:

			uiProbDenFuncGenGaussian(uiParent*,int nrdim,MultiID&,
						 const MnemonicSelection&,
						 const BufferStringSet&);
			~uiProbDenFuncGenGaussian();

    MultiID&		ioobjky_;

    ProbDenFunc*	pdf_;
    uiEditGaussianProbDenFunc* pdffld_;
    uiIOObjSel*		outfld_;

    bool		acceptOK(CallBacker*) override;

};



uiProbDenFuncGen::uiProbDenFuncGen( uiParent* p,
				    const MnemonicSelection* mns,
				    const BufferStringSet* fullnms, int defidx )
    : uiDialog(p,Setup(tr("Generate a PDF"),mNoDlgTitle,
		 mODHelpKey(mProbGenFuncGenHelpID) ))
    , defidx_(defidx)
{

    choicefld_ = new uiCheckList( this, uiCheckList::OneOnly );
    choicefld_->addItem( tr("Create an editable PDF filled "
			    "with Gaussian values"),
			    "createsampledgaussianprdf"  );
    choicefld_->addItem( tr("Create a full Gaussian PDF"),
			    "creategaussianprdf"  );
    choicefld_->addItem( tr("Create an empty PDF to edit by hand"),
			    "createuniformprdf" );
    mAttachCB( choicefld_->changed, uiProbDenFuncGen::choiceSel );

    auto* lsb = new uiLabeledSpinBox( this,
				      tr("Number of dimensions (variables)") );
    const int defnrdims = 2;
    if ( mns && !mns->isEmpty() && mns->size() < defmaxnrdims_ )
	defmaxnrdims_ = mns->size();
    lsb->attach( ensureBelow, choicefld_ );
    nrdimfld_ = lsb->box();
    nrdimfld_->setInterval( 1, defmaxnrdims_, 1 );
    mAttachCB( nrdimfld_->valueChanging, uiProbDenFuncGen::nrDimsChgCB );
    nrdimfld_->setValue( defnrdims );

    uiMnemonicsSel* prevmnsel = nullptr;
    const int defmaxnrdims = mns ? mns->size() : defmaxnrdims_;
    for ( int idx=0; idx<defmaxnrdims; idx++ )
    {
	const uiString lbl = tr("Variable #%1").arg( idx+1 );
	auto* mnsel = new uiMnemonicsSel( this, uiMnemonicsSel::Setup(mns,lbl));
	if ( fullnms )
	    mnsel->setNames( *fullnms );

	if ( idx < mnsel->box()->size() )
	    mnsel->box()->setCurrentItem( idx );
	mnsel->attach( ensureBelow, prevmnsel ? (uiObject*)prevmnsel
					      : (uiObject*)lsb );
	prevmnsel = mnsel;
	mnsels_.add( mnsel );
    }
    if ( defidx_ >= mnsels_.size() && !mnsels_.isEmpty() )
	mnsels_.first()->box()->setCurrentItem( defidx );

    mAttachCB( postFinalize(), uiProbDenFuncGen::initDlg );
}


uiProbDenFuncGen::~uiProbDenFuncGen()
{
    detachAllNotifiers();
}


void uiProbDenFuncGen::initDlg( CallBacker* cb )
{
    nrDimsChgCB( cb );
}


void uiProbDenFuncGen::nrDimsChgCB( CallBacker* )
{
    const int nrdims = nrdimfld_->getIntValue();
    for ( int idx=0; idx<nrdims; idx++ )
    {
	if ( mnsels_.validIdx(idx) )
	    mnsels_.get( idx )->display( true );
    }

    for ( int idx=nrdims; idx<mnsels_.size(); idx++ )
	mnsels_.get( idx )->display( false );
}


void uiProbDenFuncGen::choiceSel( CallBacker* )
{
    const int choice = choicefld_->firstChecked();
    const bool isfullgauss = choice == 1;
    int maxnrdims = defmaxnrdims_;
    if ( isfullgauss )
    {
	maxnrdims = mnsels_.isEmpty() ? 20
		  : mnsels_.first()->getSelection().size();
    }

    nrdimfld_->setInterval( 1, maxnrdims, 1 );
    nrDimsChgCB( nullptr );
}


bool uiProbDenFuncGen::acceptOK( CallBacker* )
{
    const int choice = choicefld_->firstChecked();
    const int nrdims = nrdimfld_->getIntValue();

    MnemonicSelection mns;
    BufferStringSet varnms;
    for ( auto* uimnsel : mnsels_ )
    {
	if ( !uimnsel->isDisplayed() )
	    break;

	mns.add( uimnsel->mnemonic() );
	varnms.add( uimnsel->box()->text() );
    }

    if ( mns.size() < varnms.size() )
    {
	mns.setEmpty();
	varnms.setEmpty();
    }

    const bool isfullgauss = choice == 1;
    if ( isfullgauss )
    {
	uiProbDenFuncGenGaussian dlg( this, nrdims, ioobjky_, mns, varnms );
	return dlg.go();
    }

    const bool isgauss = choice == 0;
    uiProbDenFuncGenSampled dlg( this, nrdims, isgauss, ioobjky_, mns, varnms );
    return dlg.go();
}



uiProbDenFuncGenSampled::uiProbDenFuncGenSampled( uiParent* p, int nrdim,
						  bool isgauss, MultiID& ky,
						  const MnemonicSelection& mns,
						  const BufferStringSet& varnms)
    : uiDialog(p,Setup(tr("Generate editable PDF"),mNoDlgTitle,
		       mODHelpKey(mProbDenFuncGenSampledHelpID)))
    , nrdims_(nrdim)
    , ioobjky_(ky)
{
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	auto* nmfld = new uiGenInput( this, nrdims_ == 1 ?
		tr("Variable name") : tr("Dimension %1: Name").arg(idx+1) );
	BufferString varnm;
	Interval<float> rg = Interval<float>::udf();
	const UnitOfMeasure* uom = nullptr;
	uiEditProbDenFunc::getPars( &mns, &varnms, idx, varnm, rg, uom );
	if ( !varnm.isEmpty() )
	    nmfld->setText( varnm );

	auto* rgfld = new uiGenInput(this, mJoinUiStrs(sValue(),sRange()),
				FloatInpSpec(), FloatInpSpec() );
	if ( !rg.isUdf() )
	    rgfld->setValue( rg );
	mAttachCB( rgfld->valueChanged, uiProbDenFuncGenSampled::rgChg );

	uiUnitSel::Setup ussu( Mnemonic::Other );
	ussu.mn( mns.validIdx(idx) ? mns.get(idx) : nullptr )
	    .mode( uiUnitSel::Setup::SymbolsOnly ).withnone( !uom );
	auto* unitfld = new uiUnitSel( this, ussu );
	unitfld->setUnit( uom );
	mAttachCB( unitfld->selChange, uiProbDenFuncGenSampled::unitChgCB );

	nmflds_ += nmfld;
	rgflds_ += rgfld;
	unitflds_ += unitfld;
	if ( idx )
	    nmfld->attach( alignedBelow, nmflds_[idx-1] );
	rgfld->attach( rightOf, nmfld );
	unitfld->attach( rightOf, rgfld );
    }

    auto* lsb = new uiLabeledSpinBox( this,
      nrdims_ == 1 ? tr("Number of bins") : tr("Number of bins per dimension"));
    nrbinsfld_ = lsb->box();
    nrbinsfld_->setInterval( 3, 10000, 1 );
    nrbinsfld_->setValue( 25 );
    lsb->attach( alignedBelow, nmflds_[nmflds_.size()-1] );

    uiGroup* alfld = lsb;
    if ( isgauss )
    {
	for ( int idx=0; idx<nrdims_; idx++ )
	{
	    uiString lbltxt =  nrdims_ == 1 ? tr("Exp/Std") :
							uiStrings::sDimension();
	    if ( nrdims_ > 1 )
		lbltxt = toUiString("%1 %2 %3").arg(lbltxt).arg(idx+1 )
					       .arg( tr(": Exp/Std") );
	    auto* expstdfld = new uiGenInput( this, lbltxt,
					      FloatInpSpec(), FloatInpSpec() );
	    expstdfld->attach( alignedBelow, alfld );
	    expstdflds_ += expstdfld;
	    alfld = expstdfld;
	}

#define	mMkCorrFld(s,pos,att) \
	fld = new uiGenInput( this, s, FloatInpSpec(0) ); \
	fld->setElemSzPol( uiObject::Small ); \
	fld->attach( pos, att ); \
	ccflds_ += fld;
	if ( nrdims_ > 1 )
	{
	    uiGenInput* fld;
	    mMkCorrFld(nrdims_ == 2 ? uiStrings::sCorrelation():tr("%1 1 -> 2")
		      .arg(uiStrings::sCorrelation()), rightOf, expstdflds_[1]);
	    if ( nrdims_ > 2 )
	    {
		mMkCorrFld( tr("%1 1 -> 3").arg(uiStrings::sCorrelation()),
						     alignedBelow, ccflds_[0] );
		mMkCorrFld( tr("%1 2 -> 3").arg(uiStrings::sCorrelation()),
						     rightOf, ccflds_[1] );
	    }
	}
    }

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, alfld );

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt );
    outfld_->attach( alignedBelow, alfld );
    outfld_->attach( ensureBelow, sep );

    mAttachCB( postFinalize(), uiProbDenFuncGenSampled::initDlg );
}


uiProbDenFuncGenSampled::~uiProbDenFuncGenSampled()
{
    detachAllNotifiers();
}


void uiProbDenFuncGenSampled::initDlg( CallBacker* )
{
    for ( auto* rgfld : rgflds_ )
    {
	const Interval<float> rg = rgfld->getFInterval();
	if ( !rg.isUdf() )
	    rgChg( rgfld );
    }
}


void uiProbDenFuncGenSampled::unitChgCB( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpackWithCaller(const UnitOfMeasure*,prevuom,cber,cb);
    mDynamicCastGet(uiUnitSel*,uiunitsel,cber)
    if ( !uiunitsel )
	return;

    const int dimidx = unitflds_.indexOf( uiunitsel );
    if ( !unitflds_.validIdx(dimidx) )
	return;

    const UnitOfMeasure* newuom = uiunitsel->getUnit();
    uiGenInput* rgfld = rgflds_.get( dimidx );
    NotifyStopper ns( rgfld->valueChanged );
    const Interval<float> rg( rgfld->getFInterval() );
    rgfld->setValue( getConvertedValue( rg.start, prevuom, newuom ), 0 );
    rgfld->setValue( getConvertedValue( rg.stop, prevuom, newuom ), 1 );

    if ( !isGauss() )
	return;

    uiGenInput* expstdfld = expstdflds_[dimidx];
    const Interval<float> exp( expstdfld->getFInterval() );
    expstdfld->setValue( getConvertedValue( exp.start, prevuom, newuom ), 0 );
    expstdfld->setValue( getConvertedValue( exp.stop, prevuom, newuom ), 1 );
}


void uiProbDenFuncGenSampled::rgChg( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,ginp,cb)
    if ( !ginp )
	{ pErrMsg("Huh? ginp null"); return; }
    const int dimidx = rgflds_.indexOf( ginp );
    if ( dimidx < 0 )
	{ pErrMsg("Huh? dimidx<0"); return; }

    if ( !isGauss() )
	return;

    uiGenInput* expstdfld = expstdflds_[dimidx];
    if ( !expstdfld->isUndef(0) )
	return;

    const Interval<float> rg( ginp->getFInterval() );
    if ( rg.isUdf() )
	return;

    expstdfld->setValue( rg.center(), 0 );
    expstdfld->setValue( rg.width()/4., 1 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

#define mGetCCValue(ifld) \
{ \
    float cc = ccflds_[ifld]->getFValue(); \
    if ( mIsUdf(cc) ) cc = 0; \
    if ( cc < -cMaxGaussianCC() || cc > cMaxGaussianCC() ) \
	mErrRet( mToUiStringTodo(sGaussianCCRangeErrMsg()) ) \
    ccs_ += cc; \
}


bool uiProbDenFuncGenSampled::getFromScreen()
{
    nrbins_= nrbinsfld_->getIntValue();
    od_int64 totalbins = nrbins_;
    totalbins = Math::IntPowerOf( totalbins, nrdims_ );
    if (totalbins > 100000
	&& !uiMSG().askGoOn(tr("You have requested a total of %1"
			       " bins.\nAre you sure this is what you want?")
		       .arg(totalbins)))
	return false;

    dimnms_.setEmpty(); rgs_.setEmpty();
    exps_.setEmpty(); stds_.setEmpty(); ccs_.setEmpty();
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	dimnms_.add( nmflds_[idim]->text() );
	if ( dimnms_.get(idim).isEmpty() )
	    mErrRet(tr("Please enter a name for each variable"))

	Interval<float> rg;
	rg.start = rgflds_[idim]->getFValue(0);
	rg.stop = rgflds_[idim]->getFValue(1);
	if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	    mErrRet(tr("Please fill all variable ranges"))
	rg.sort();
	rgs_ += rg;

	if ( !isGauss() )
	    continue;

	const float exp = expstdflds_[idim]->getFValue(0);
	const float stdev = expstdflds_[idim]->getFValue(1);
	if ( mIsUdf(exp) || mIsUdf(stdev) )
	    mErrRet(tr("Please fill all expectations and standard deviations"))
	exps_ += exp; stds_ += stdev;

	if ( idim == 1 )
	    mGetCCValue( 0 )
	if ( idim == 2 )
	    { mGetCCValue( 1 ) mGetCCValue( 2 ) }
    }

    const IOObj* pdfioobj = outfld_->ioobj();
    if ( !pdfioobj )
	return false;

    ioobjky_ = pdfioobj->key();
    return true;
}


// Note that a Sampled PDF's SD starts at the center of a bin
// Thus, to get user's range, we need to start and stop half a step inward
#define mSetSD( sd, rg ) \
	spdf->sd.step = (rg.stop-rg.start) / nrbins_; \
	spdf->sd.start = rg.start + spdf->sd.step / 2


ProbDenFunc* uiProbDenFuncGenSampled::getPDF() const
{
    Stats::CalcSetup csu; csu.require( Stats::Max );
    Stats::RunCalc<float> rc( csu );
    ProbDenFunc* ret = nullptr;
    int nrdims = nrdims_;
    if ( nrdims > 3 )
	nrdims = 3;
    const TypeSet<int> szs( nrdims, nrbins_ );
    if ( nrdims_ == 1 )
    {
	auto* spdf = new Sampled1DProbDenFunc;
	spdf->setSize( szs );
	spdf->getData().setAll( mUdf(float) );
	spdf->setDimName( 0, dimnms_.get(0) );
	spdf->setUOMSymbol( 0, unitflds_.first()->getUnitName() );
	mSetSD( sampling(), rgs_[0] );
	if ( !isGauss() )
	    return spdf;

	Gaussian1DProbDenFunc gpdf;
	gpdf.set( exps_[0], stds_[0] );
	for ( int idx=0; idx<nrbins_; idx++ )
	{
	    const float pos = spdf->sampling().atIndex( idx );
	    const float val = gpdf.value( pos );
	    spdf->set( idx, val );
	    rc.addValue( val );
	}

	ret = spdf;
    }
    else if ( nrdims_ == 2 )
    {
	auto* spdf = new Sampled2DProbDenFunc;
	spdf->setSize( szs );
	spdf->getData().setAll( mUdf(float) );
	mSetSD( sampling(0), rgs_[0] );
	mSetSD( sampling(1), rgs_[1] );
	spdf->setDimName( 0, dimnms_.get(0) );
	spdf->setDimName( 1, dimnms_.get(1) );
	spdf->setUOMSymbol( 0, unitflds_.get(0)->getUnitName() );
	spdf->setUOMSymbol( 1, unitflds_.get(1)->getUnitName() );
	spdf->getData().setAll( mUdf(float) );
	if ( !isGauss() )
	    return spdf;

	Gaussian2DProbDenFunc gpdf;
	gpdf.set( 0, exps_[0], stds_[0] );
	gpdf.set( 1, exps_[1], stds_[1] );
	gpdf.setCorrelation( ccs_[0] );

	for ( int i0=0; i0<nrbins_; i0++ )
	{
	    const float p0 = spdf->sampling(0).atIndex( i0 );
	    for ( int i1=0; i1<nrbins_; i1++ )
	    {
		const float p1 = spdf->sampling(1).atIndex( i1 );
		const float val = gpdf.value( p0, p1 );
		spdf->set( i0, i1, val );
		rc.addValue( val );
	    }
	}

	ret = spdf;
    }
    else
    {
	auto* spdf = new SampledNDProbDenFunc( 3 );
	spdf->setSize( szs );
	spdf->getData().setAll( mUdf(float) );
	for ( int idx=0; idx<3; idx++ )
	{
	    mSetSD( sampling(idx), rgs_[idx] );
	    spdf->setDimName( idx, dimnms_.get( idx ) );
	    spdf->setUOMSymbol( idx, unitflds_.get(idx)->getUnitName() );
	}
	if ( !isGauss() )
	    return spdf;

	GaussianNDProbDenFunc gpdf( 3 );
	for ( int idim=0; idim<nrdims_; idim++ )
	{
	    gpdf.setDimName( idim, dimnms_.get(idim) );
	    gpdf.vars_[idim] =
		GaussianNDProbDenFunc::VarDef( exps_[idim], stds_[idim] );
	}

	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 0, 1, ccs_[0] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 0, 2, ccs_[1] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 1, 2, ccs_[2] );

	TypeSet<float> poss( 3, 0.f );
	TypeSet<int> idxs( 3, 0 );
	for ( idxs[0]=0; idxs[0]<nrbins_; idxs[0]++ )
	{
	    poss[0] = spdf->sampling(0).atIndex( idxs[0] );
	    for ( idxs[1]=0; idxs[1]<nrbins_; idxs[1]++ )
	    {
		poss[1] = spdf->sampling(1).atIndex( idxs[1] );
		for ( idxs[2]=0; idxs[2]<nrbins_; idxs[2]++ )
		{
		    poss[2] = spdf->sampling(2).atIndex( idxs[2] );
		    const float val = gpdf.value( poss );
		    spdf->setND( idxs.arr(), val );
		    rc.addValue( val );
		}
	    }
	}

	ret = spdf;
    }

    const float maxval = rc.max();
    if ( maxval )
	ret->scale( cMaxSampledBinVal / maxval );

    return ret;
}


bool uiProbDenFuncGenSampled::acceptOK( CallBacker* )
{
    if ( !getFromScreen() )
	return false;

    PtrMan<ProbDenFunc> pdf = getPDF();
    if ( !pdf )
	return false;

    return writePDF( *pdf, *outfld_->ioobj() );
}



uiProbDenFuncGenGaussian::uiProbDenFuncGenGaussian( uiParent* p, int nrdim,
						MultiID& ky,
						const MnemonicSelection& mns,
						const BufferStringSet& varnms )
    : uiDialog(p,Setup(tr("Generate Gaussian PDF"),mNoDlgTitle,
			mODHelpKey(mProbDenFuncGenGaussianHelpID)))
    , ioobjky_(ky)
{
    if ( nrdim == 1 )
	pdf_ = new Gaussian1DProbDenFunc;
    else if ( nrdim == 2 )
	pdf_ = new Gaussian2DProbDenFunc;
    else
	pdf_ = new GaussianNDProbDenFunc( nrdim );

    pdffld_ = new uiEditGaussianProbDenFunc( this, *pdf_, true,
					     true, &mns, &varnms );

    uiSeparator* sep = nullptr;
    if ( nrdim < 3 )
    {
	sep = new uiSeparator( this );
	sep->attach( stretchedBelow, pdffld_ );
    }

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt );
    if ( !sep )
	outfld_->attach( ensureBelow, pdffld_ );
    else
    {
	outfld_->attach( alignedBelow, pdffld_ );
	outfld_->attach( ensureBelow, sep );
    }
}


uiProbDenFuncGenGaussian::~uiProbDenFuncGenGaussian()
{
    delete pdf_;
}


bool uiProbDenFuncGenGaussian::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = outfld_->ioobj();
    if ( !pdfioobj || !pdffld_->commitChanges() )
	return false;

    ioobjky_ = pdfioobj->key();
    return writePDF( *pdf_, *pdfioobj );
}

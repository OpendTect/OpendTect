/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprobdenfuncgen.h"

#include "uigeninput.h"
#include "uitabstack.h"
#include "uispinbox.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uichecklist.h"
#include "uitoolbutton.h"
#include "uiioobjsel.h"
#include "uiseparator.h"
#include "uitabstack.h"
#include "uilabel.h"
#include "uimsg.h"

#include "sampledprobdenfunc.h"
#include "gaussianprobdenfunc.h"
#include "probdenfunctr.h"
#include "statruncalc.h"

static const float cMaxCC = 0.99999f; // if changed, also change error message
static const float cMaxProbVal = 100.0f;
static const char* sCCRangeErr = "Correlation coefficients should be "
		    "in range <-1,1>.\nMaximum correlation is 0.99999.";


class uiProbDenFuncGenSampled : public uiDialog
{
public:

			uiProbDenFuncGenSampled(uiParent*,int nrdim,bool gauss,
						MultiID&);

    uiSpinBox*		nrbinsfld_;
    ObjectSet<uiGenInput> nmflds_;
    ObjectSet<uiGenInput> rgflds_;
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

    inline bool		isGauss() const	{ return !expstdflds_.isEmpty(); }
    bool		getFromScreen();
    ProbDenFunc*	getPDF() const;

    bool		acceptOK(CallBacker*);
    void		chgCB(CallBacker*);
    void		rgChg(CallBacker*);
};


class uiProbDenFuncGenGaussian : public uiDialog
{
public:

			uiProbDenFuncGenGaussian(uiParent*,int nrdim,MultiID&);

    const int		nrdims_;
    MultiID&		ioobjky_;
    Gaussian1DProbDenFunc* pdf1d_;
    Gaussian2DProbDenFunc* pdf2d_;
    GaussianNDProbDenFunc* pdfnd_;

    uiTabStack*		tabstack_;
    uiGenInput*		ccfld_;
    ObjectSet<uiGenInput> nmflds_;
    ObjectSet<uiGenInput> expflds_;
    ObjectSet<uiGenInput> stdflds_;
    uiComboBox*		var1fld_;
    uiComboBox*		var2fld_;
    uiPushButton*	addsetbut_;
    uiListBox*		defcorrsfld_;
    uiToolButton*	rmbut_;
    uiIOObjSel*		outfld_;

    float		getCC() const;
    void		mkCorrTabFlds(uiGroup*);
    int			findCorr() const;
    void		updateCorrList(int);

    void		tabChg(CallBacker*);
    void		corrSel(CallBacker*);
    void		varSel(CallBacker*);
    void		addSetPush(CallBacker*);
    void		rmPush(CallBacker*);

    bool		acceptOK(CallBacker*);

};



uiProbDenFuncGen::uiProbDenFuncGen( uiParent* p )
    : uiDialog(p,Setup("Generate a PDF",mNoDlgTitle,"112.1.2"))
{

    choicefld_ = new uiCheckList( this, uiCheckList::OneOnly );
    choicefld_->addItem( "Create an editable PDF &filled with Gaussian values");
    choicefld_->addItem( "Create a full &Gaussian PDF" );
    choicefld_->addItem( "Create an &empty PDF to edit by hand" );
    choicefld_->changed.notify( mCB(this,uiProbDenFuncGen,choiceSel) );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
				"Number of dimensions (variables)" );
    lsb->attach( ensureBelow, choicefld_ );
    nrdimfld_ = lsb->box();
    nrdimfld_->setInterval( 1, 3, 1 );
    nrdimfld_->setValue( 2 );
}


void uiProbDenFuncGen::choiceSel( CallBacker* )
{
    const int choice = choicefld_->firstChecked();
    const bool isfullgauss = choice == 1;
    nrdimfld_->setInterval( 1, isfullgauss ? 20 : 3, 1 );
}


bool uiProbDenFuncGen::acceptOK( CallBacker* )
{
    const int choice = choicefld_->firstChecked();
    const int nrdims = nrdimfld_->getValue();

    if ( choice == 1 )
    {
	uiProbDenFuncGenGaussian dlg( this, nrdims, ioobjky_ );
	return dlg.go();
    }

    uiProbDenFuncGenSampled dlg( this, nrdims, choice==0, ioobjky_ );
    return dlg.go();
}



uiProbDenFuncGenSampled::uiProbDenFuncGenSampled( uiParent* p, int nrdim,
						  bool isgauss, MultiID& ky )
    : uiDialog(p,Setup("Generate editable PDF",mNoDlgTitle,mTODOHelpKey))
    , nrdims_(nrdim)
    , ioobjky_(ky)
{
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	uiGenInput* nmfld = new uiGenInput( this, nrdims_ == 1 ?
		"Variable name" : BufferString("Dimension ",idx+1,": Name") );
	uiGenInput* rgfld = new uiGenInput( this, "Value range",
				FloatInpSpec(), FloatInpSpec() );
	rgfld->valuechanged.notify( mCB(this,uiProbDenFuncGenSampled,rgChg) );
	nmflds_ += nmfld; rgflds_ += rgfld;
	if ( idx )
	    nmfld->attach( alignedBelow, nmflds_[idx-1] );
	rgfld->attach( rightOf, nmfld );
    }

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
	    nrdims_ == 1 ? "Number of bins" : "Number of bins per dimension");
    nrbinsfld_ = lsb->box();
    nrbinsfld_->setInterval( 3, 10000, 1 );
    nrbinsfld_->setValue( 25 );
    lsb->attach( alignedBelow, nmflds_[nmflds_.size()-1] );

    uiGroup* alfld = lsb;
    if ( isgauss )
    {
	for ( int idx=0; idx<nrdims_; idx++ )
	{
	    BufferString lbltxt( nrdims_ == 1 ? "Exp/Std" : "Dimension " );
	    if ( nrdims_ > 1 )
		lbltxt.add( idx+1 ).add( ": Exp/Std" );
	    uiGenInput* expstdfld = new uiGenInput( this, lbltxt,
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
	    mMkCorrFld( nrdims_ == 2 ? "Correlation" : "Correlation 1 -> 2",
			rightOf, expstdflds_[1] );
	    if ( nrdims_ > 2 )
	    {
		mMkCorrFld( "Correlation 1 -> 3", alignedBelow, ccflds_[0] );
		mMkCorrFld( "Correlation 2 -> 3", rightOf, ccflds_[1] );
	    }
	}
    }

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, alfld );

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt );
    outfld_->attach( alignedBelow, alfld );
    outfld_->attach( ensureBelow, sep );
}


void uiProbDenFuncGenSampled::rgChg( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,ginp,cb)
    if ( !ginp )
	{ pErrMsg("Huh? ginp null"); return; }
    const int dimidx = rgflds_.indexOf( ginp );
    if ( dimidx < 0 )
	{ pErrMsg("Huh? dimidx<0"); return; }
    uiGenInput* stdfld = expstdflds_[dimidx];
    if ( !stdfld->isUndef(0) )
	return;

    const Interval<float> rg( ginp->getFInterval() );
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	return;

    stdfld->setValue( rg.center(), 0 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

#define mGetCCValue(ifld) \
{ \
    float cc = ccflds_[ifld]->getfValue(); \
    if ( mIsUdf(cc) ) cc = 0; \
    if ( cc < -cMaxCC || cc > cMaxCC ) \
	mErrRet( sCCRangeErr ) \
    ccs_ += cc; \
}


bool uiProbDenFuncGenSampled::getFromScreen()
{
    nrbins_= nrbinsfld_->getValue();
    od_int64 totalbins = nrbins_;
    totalbins = Math::IntPowerOf( totalbins, nrdims_ );
    if ( totalbins > 100000
      && !uiMSG().askGoOn(BufferString("You have requested a total of ",
	      totalbins, " bins.\nAre you sure this is what you want?")) )
	return false;

    dimnms_.setEmpty(); rgs_.setEmpty();
    exps_.setEmpty(); stds_.setEmpty(); ccs_.setEmpty();
    for ( int idim=0; idim<nrdims_; idim++ )
    {
	dimnms_.add( nmflds_[idim]->text() );
	if ( dimnms_.get(idim).isEmpty() )
	    mErrRet( "Please enter a name for each variable" )

	float exp = expstdflds_[idim]->getfValue(0);
	float stdev = expstdflds_[idim]->getfValue(1);
	if ( mIsUdf(exp) || mIsUdf(stdev) )
	    mErrRet( "Please fill all expectations and standard deviations" )
	exps_ += exp; stds_ += stdev;

	Interval<float> rg;
	rg.start = rgflds_[idim]->getfValue(0);
	rg.stop = rgflds_[idim]->getfValue(1);
	if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	    mErrRet( "Please fill all variable ranges" )
	rg.sort();
	rgs_ += rg;

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
    ProbDenFunc* ret = 0;
    if ( nrdims_ == 1 )
    {
	Gaussian1DProbDenFunc gpdf( exps_[0], stds_[0] );
	Sampled1DProbDenFunc* spdf = new Sampled1DProbDenFunc;
	spdf->bins_.setSize( nrbins_ );
	mSetSD( sd_, rgs_[0] );
	for ( int idx=0; idx<nrbins_; idx++ )
	{
	    const float pos = spdf->sd_.atIndex( idx );
	    const float val = gpdf.value( pos );
	    spdf->bins_.set( idx, val );
	    rc.addValue( val );
	}
	spdf->setDimName( 0, dimnms_.get(0) );
	ret = spdf;
    }
    else if ( nrdims_ == 2 )
    {
	Gaussian2DProbDenFunc gpdf;
	gpdf.exp0_ = exps_[0]; gpdf.std0_ = stds_[0];
	gpdf.exp1_ = exps_[1]; gpdf.std1_ = stds_[1];
	gpdf.cc_ = ccs_[0];

	Sampled2DProbDenFunc* spdf = new Sampled2DProbDenFunc;
	spdf->bins_.setSize( nrbins_, nrbins_ );
	mSetSD( sd0_, rgs_[0] );
	mSetSD( sd1_, rgs_[1] );
	for ( int i0=0; i0<nrbins_; i0++ )
	{
	    const float p0 = spdf->sd0_.atIndex( i0 );
	    for ( int i1=0; i1<nrbins_; i1++ )
	    {
		const float p1 = spdf->sd1_.atIndex( i1 );
		const float val = gpdf.value( p0, p1 );
		spdf->bins_.set( i0, i1, val );
		rc.addValue( val );
	    }
	}
	spdf->setDimName( 0, dimnms_.get(0) );
	spdf->setDimName( 1, dimnms_.get(1) );
	ret = spdf;
    }
    else
    {
	GaussianNDProbDenFunc gpdf( 3 );
	for ( int idim=0; idim<nrdims_; idim++ )
	    gpdf.vars_[idim] = GaussianNDProbDenFunc::VarDef( dimnms_.get(idim),
						exps_[idim], stds_[idim] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 0, 1, ccs_[0] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 0, 2, ccs_[1] );
	gpdf.corrs_ += GaussianNDProbDenFunc::Corr( 1, 2, ccs_[2] );

	SampledNDProbDenFunc* spdf = new SampledNDProbDenFunc( 3 );
	const TypeSet<int> szs( 3, nrbins_ );
	spdf->bins_.setSize( szs.arr() );
	mSetSD( sds_[0], rgs_[0] );
	mSetSD( sds_[1], rgs_[1] );
	mSetSD( sds_[2], rgs_[2] );

	TypeSet<float> poss( 3, 0.0f );
	TypeSet<int> idxs( 3, 0 );
	for ( idxs[0]=0; idxs[0]<nrbins_; idxs[0]++ )
	{
	    poss[0] = spdf->sds_[0].atIndex( idxs[0] );
	    for ( idxs[1]=0; idxs[1]<nrbins_; idxs[1]++ )
	    {
		poss[1] = spdf->sds_[1].atIndex( idxs[1] );
		for ( idxs[2]=0; idxs[2]<nrbins_; idxs[2]++ )
		{
		    poss[2] = spdf->sds_[2].atIndex( idxs[2] );
		    const float val = gpdf.value( poss );
		    spdf->bins_.setND( idxs.arr(), val );
		    rc.addValue( val );
		}
	    }
	}

	spdf->dimnms_ = dimnms_;
	ret = spdf;
    }

    const float maxval = rc.max();
    if ( maxval )
	ret->scale( cMaxProbVal / maxval );

    return ret;
}


bool uiProbDenFuncGenSampled::acceptOK( CallBacker* )
{
    if ( !getFromScreen() )
	return false;

    PtrMan<ProbDenFunc> pdf = getPDF();
    if ( !pdf )
	return false;

    if ( !ProbDenFuncTranslator::write(*pdf,*outfld_->ioobj()) )
	mErrRet("Could not write PDF to disk")

    return true;
}



uiProbDenFuncGenGaussian::uiProbDenFuncGenGaussian( uiParent* p, int nrdim,
						    MultiID& ky )
    : uiDialog(p,Setup("Generate Gaussian PDF",mNoDlgTitle,mTODOHelpKey))
    , nrdims_(nrdim)
    , ioobjky_(ky)
    , tabstack_(0)
    , ccfld_(0)
    , pdf1d_(0)
    , pdf2d_(0)
    , pdfnd_(0)
{
    if ( nrdims_ == 1 )
	pdf1d_ = new Gaussian1DProbDenFunc;
    else if ( nrdims_ == 2 )
	pdf2d_ = new Gaussian2DProbDenFunc;
    else
	pdfnd_ = new GaussianNDProbDenFunc( nrdims_ );

    uiGroup* varsgrp = 0;
    if ( !pdfnd_ )
	varsgrp = new uiGroup( this, "Vars group" );
    else
    {
	tabstack_ = new uiTabStack( this, "Tabs" );
	varsgrp = new uiGroup( tabstack_->tabGroup(), "Vars group" );
    }

    for ( int idim=0; idim<nrdims_; idim++ )
    {
	BufferString varnm;
	if ( !pdf1d_ )
	    varnm.set( "Var " ).add( idim+1 );
	uiGenInput* nmfld = new uiGenInput( varsgrp, "", StringInpSpec(varnm) );
	uiGenInput* expfld = new uiGenInput( varsgrp, "", FloatInpSpec() );
	uiGenInput* stdfld = new uiGenInput( varsgrp, "", FloatInpSpec() );
	if ( idim > 0 )
	    nmfld->attach( alignedBelow, nmflds_[idim-1] );
	else
	{
	    varsgrp->setHAlignObj( expfld );
	    uiLabel* lbl = new uiLabel( varsgrp, "Variable name" );
	    lbl->attach( centeredAbove, nmfld );
	    lbl = new uiLabel( varsgrp, "Expectation" );
	    lbl->attach( centeredAbove, expfld );
	    lbl = new uiLabel( varsgrp, "Standard Deviation" );
	    lbl->attach( centeredAbove, stdfld );
	}
	expfld->attach( rightOf, nmfld );
	stdfld->attach( rightOf, expfld );
	nmflds_ += nmfld; expflds_ += expfld; stdflds_ += stdfld;
    }

    if ( !pdf1d_ )
    {
	uiGroup* ccgrp = 0;
	if ( pdf2d_ )
	{
	    ccfld_ = new uiGenInput( this, "Correlation", FloatInpSpec(0));
	    ccfld_->attach( alignedBelow, varsgrp );
	}
	else
	{
	    ccgrp = new uiGroup( tabstack_->tabGroup(), "CC group" );
	    mkCorrTabFlds( ccgrp );
	    tabstack_->addTab( varsgrp, "Distributions" );
	    tabstack_->addTab( ccgrp, "Correlations" );
	    tabstack_->selChange().notify(
				mCB(this,uiProbDenFuncGenGaussian,tabChg) );
	}
    }

    uiSeparator* sep = 0;
    if ( !tabstack_ )
    {
	sep = new uiSeparator( this );
	if ( pdf1d_ )
	    sep->attach( stretchedBelow, nmflds_[0] );
	else
	    sep->attach( stretchedBelow, ccfld_ );
    }

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt );
    if ( tabstack_ )
	outfld_->attach( ensureBelow, tabstack_ );
    else
    {
	outfld_->attach( alignedBelow, expflds_[0]->parent() );
	outfld_->attach( ensureBelow, sep );
    }
}


void uiProbDenFuncGenGaussian::mkCorrTabFlds( uiGroup* ccgrp )
{
    uiGroup* topgrp = new uiGroup( ccgrp, "CC top group" );
    var1fld_ = new uiComboBox( topgrp, "Var 1" );
    var2fld_ = new uiComboBox( topgrp, "Var 2" );
    ccfld_ = new uiGenInput( topgrp, "", FloatInpSpec(0));
    var2fld_->attach( rightOf, var1fld_ );
    ccfld_->attach( rightOf, var2fld_ );
    const CallBack varselcb( mCB(this,uiProbDenFuncGenGaussian,varSel) );
    var1fld_->selectionChanged.notify( varselcb );
    var2fld_->selectionChanged.notify( varselcb );
    uiLabel* lbl = new uiLabel( topgrp, "Correlate" );
    lbl->attach( centeredAbove, var1fld_ );
    lbl = new uiLabel( topgrp, "With" );
    lbl->attach( centeredAbove, var2fld_ );
    lbl = new uiLabel( topgrp, "Coefficient" );
    lbl->attach( centeredAbove, ccfld_ );

    addsetbut_ = new uiPushButton( ccgrp, "&Add",
	   		mCB(this,uiProbDenFuncGenGaussian,addSetPush), true );
    addsetbut_->attach( centeredBelow, topgrp );

    defcorrsfld_ = new uiListBox( ccgrp, "Defined Correlations", false );
    defcorrsfld_->attach( centeredBelow, addsetbut_ );
    defcorrsfld_->setStretch( 2, 2 );
    defcorrsfld_->selectionChanged.notify(
	    			mCB(this,uiProbDenFuncGenGaussian,corrSel) );

    rmbut_ = new uiToolButton( ccgrp, "trashcan",
	    			"Remove selected correlation",
				mCB(this,uiProbDenFuncGenGaussian,rmPush) );
    rmbut_->attach( rightOf, defcorrsfld_ );
}


int uiProbDenFuncGenGaussian::findCorr() const
{
    const int idx0 = var1fld_->currentItem();
    const int idx1 = var2fld_->currentItem();
    for ( int icorr=0; icorr<pdfnd_->corrs_.size(); icorr++ )
    {
	const GaussianNDProbDenFunc::Corr& corr = pdfnd_->corrs_[icorr];
	if ( (corr.idx0_ == idx0 && corr.idx1_ == idx1)
	  || (corr.idx1_ == idx0 && corr.idx0_ == idx1) )
	    return icorr;
    }
    return -1;
}


void uiProbDenFuncGenGaussian::updateCorrList( int cursel )
{
    NotifyStopper stopper( defcorrsfld_->selectionChanged );
    defcorrsfld_->setEmpty();
    if ( pdfnd_->corrs_.isEmpty() )
	return;

    const int nrcorrs = pdfnd_->corrs_.size();

    for ( int icorr=0; icorr<nrcorrs; icorr++ )
    {
	GaussianNDProbDenFunc::Corr corr = pdfnd_->corrs_[icorr];
	BufferString itmtxt( var1fld_->textOfItem(corr.idx0_), " <-> " );
	itmtxt.add( var1fld_->textOfItem(corr.idx1_) )
	      .add( " (" ).add( corr.cc_ ).add( ")" );
	defcorrsfld_->addItem( itmtxt );
    }

    if ( cursel >= nrcorrs )
	cursel = nrcorrs - 1;
    else if ( cursel < 0 )
	cursel = 0;

    stopper.restore();
    defcorrsfld_->setCurrentItem( cursel );
}


void uiProbDenFuncGenGaussian::tabChg( CallBacker* )
{
    if ( !tabstack_ ) return;

    BufferStringSet varnms;
    for ( int idx=0; idx<nmflds_.size(); idx++ )
    {
	GaussianNDProbDenFunc::VarDef& vd = pdfnd_->vars_[idx];
	vd.name_ = nmflds_[idx]->text();
	vd.exp_ = expflds_[idx]->getfValue();
	vd.std_ = stdflds_[idx]->getfValue();
	varnms.add( vd.name_ );
    }

    NotifyStopper stopper1( var1fld_->selectionChanged );
    NotifyStopper stopper2( var2fld_->selectionChanged );
    var1fld_->setEmpty(); var2fld_->setEmpty();
    var1fld_->addItems( varnms ); var2fld_->addItems( varnms );

    corrSel( 0 );
}


void uiProbDenFuncGenGaussian::corrSel( CallBacker* )
{
    const int selidx = defcorrsfld_->currentItem();
    rmbut_->setSensitive( selidx >= 0 );
    if ( selidx < 0 )
	return;

    GaussianNDProbDenFunc::Corr& corr = pdfnd_->corrs_[selidx];
    var1fld_->setCurrentItem( corr.idx0_ );
    var2fld_->setCurrentItem( corr.idx1_ );
    ccfld_->setValue( corr.cc_ );

    varSel( 0 );
}


void uiProbDenFuncGenGaussian::varSel( CallBacker* cb )
{
    const int icorr = findCorr();
    addsetbut_->setText( var1fld_->currentItem() == var2fld_->currentItem()
	    ? "-" : (icorr < 0 ? "&Add" : "&Set") );
}


float uiProbDenFuncGenGaussian::getCC() const
{
    const float cc = ccfld_->getfValue();
    if ( mIsUdf(cc) )
	return cc;
    if ( cc < -cMaxCC || cc > cMaxCC )
	{ uiMSG().error( sCCRangeErr ); return mUdf(float); }
    return cc;
}


void uiProbDenFuncGenGaussian::addSetPush( CallBacker* )
{
    const int idx0 = var1fld_->currentItem();
    const int idx1 = var2fld_->currentItem();
    if ( idx0 == idx1 )
	return;
    const float cc = getCC();
    if ( mIsUdf(cc) )
	return;
    else if ( cc == 0 )
	{ uiMSG().error( "A zero correlation is not a correlation" ); return; }

    int icorr = findCorr();
    if ( icorr >= 0 )
	pdfnd_->corrs_[icorr].cc_ = cc;
    else
    {
	pdfnd_->corrs_ += GaussianNDProbDenFunc::Corr( idx0, idx1, cc );
	icorr = pdfnd_->corrs_.size() - 1;
    }

    updateCorrList( icorr );
}


void uiProbDenFuncGenGaussian::rmPush( CallBacker* )
{
    int selidx = defcorrsfld_->currentItem();
    if ( selidx >= 0 )
    {
	pdfnd_->corrs_.removeSingle( selidx );
	updateCorrList( selidx );
    }
}


bool uiProbDenFuncGenGaussian::acceptOK( CallBacker* )
{
    const IOObj* pdfioobj = outfld_->ioobj();
    if ( !pdfioobj )
	return false;

    BufferStringSet varnms;
    ProbDenFunc& pdf = *(pdf1d_ ? (ProbDenFunc*)pdf1d_
	    	: (pdf2d_ ? (ProbDenFunc*)pdf2d_ : (ProbDenFunc*)pdfnd_));
    if ( pdfnd_ )
	pdfnd_->vars_.setEmpty();

    for ( int idim=0; idim<nmflds_.size(); idim++ )
    {
	const FixedString nm = nmflds_[idim]->text();
	const float exp = expflds_[idim]->getfValue();
	const float stdev = stdflds_[idim]->getfValue();
	if ( nm.isEmpty() )
	    mErrRet("Please enter a name for all dimensions")
	else if ( varnms.isPresent(nm) )
	    mErrRet("Please enter different names for all dimensions")
	if ( mIsUdf(exp) || mIsUdf(stdev) )
	    mErrRet("Please enter all distribution values")
	if ( stdev == 0 )
	    mErrRet("Standard deviations cannot be zero")
	varnms.add( nm );

	if ( pdfnd_ )
	    pdfnd_->vars_ += GaussianNDProbDenFunc::VarDef( nm, exp, stdev );
	else
	{
	    pdf.setDimName( idim, nm );
	    if ( pdf1d_ )
		{ pdf1d_->exp_ = exp; pdf1d_->std_ = stdev; }
	    else if ( idim == 0 )
		{ pdf2d_->exp0_ = exp; pdf2d_->std0_ = stdev; }
	    else if ( idim == 1 )
		{ pdf2d_->exp1_ = exp; pdf2d_->std1_ = stdev; }
	}
    }

    if ( pdfnd_ )
	tabChg( 0 );
    else if ( pdf2d_ )
    {
	pdf2d_->cc_ = getCC();
	if ( mIsUdf(pdf2d_->cc_) )
	    return false;
    }

    if ( !ProbDenFuncTranslator::write(pdf,*pdfioobj) )
	mErrRet("Could not write PDF to disk")

    ioobjky_ = pdfioobj->key();
    return true;
}

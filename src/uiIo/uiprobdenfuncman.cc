/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprobdenfuncman.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uieditpdf.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uiseparator.h"
#include "uimsg.h"

#include "bufstring.h"
#include "sampledprobdenfunc.h"
#include "simpnumer.h"
#include "probdenfunctr.h"

static const int cPrefWidth = 75;
static const float cMaxProbVal = 100.0f;

uiProbDenFuncMan::uiProbDenFuncMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage Probability Density Functions",
				     mNoDlgTitle,
				     "112.1.0").nrstatusflds(1),
	           ProbDenFuncTranslatorGroup::ioContext())
{
    createDefaultUI();
    selgrp_->getListField()->doubleClicked.notify(
	    			mCB(this,uiProbDenFuncMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "browseprdf",
	    		 "Browse/edit this Probability Density Function",
			 mCB(this,uiProbDenFuncMan,browsePush) );
    manipgrp->addButton( "genprdf",
	    		 "Generate Probability Density Function",
			 mCB(this,uiProbDenFuncMan,genPush) );

    selgrp_->setPrefWidthInChar( mCast(float,cPrefWidth) );
    infofld_->setPrefWidthInChar( cPrefWidth );
    selChg( this );
}



uiProbDenFuncMan::~uiProbDenFuncMan()
{
}


#define mGetPDF(pdf) \
    PtrMan<ProbDenFunc> pdf = ProbDenFuncTranslator::read( *curioobj_ )


void uiProbDenFuncMan::browsePush( CallBacker* )
{
    if ( !curioobj_ ) return;
    mGetPDF(pdf);
    if ( !pdf ) return;

    uiEditProbDenFunc dlg( this, *pdf, true );
    if ( dlg.go() && dlg.isChanged() )
    {
	const int choice = uiMSG().question( "PDF changed. Save?", "&Yes",
						"&As new ...", "&No" );
	if ( choice < 0 ) return;

	PtrMan<IOObj> saveioobj = curioobj_->clone();
	if ( choice == 0 )
	{
	    CtxtIOObj ctio( ctxt_ );
	    ctio.ctxt.forread = false;
	    uiIOObjSelDlg seldlg( this, ctio, "Save As" );
	    if ( !seldlg.go() || !seldlg.ioObj() ) return;
	    saveioobj = seldlg.ioObj()->clone();
	}

	BufferString emsg;
	if ( !ProbDenFuncTranslator::write(*pdf,*saveioobj,&emsg) )
	    uiMSG().error( emsg );
	else
	    selgrp_->fullUpdate( saveioobj->key() );
    }
}


class uiProbDenFuncGen : public uiDialog
{
public:

uiProbDenFuncGen( uiParent* p )
    : uiDialog(p,Setup("Generate PDF",mNoDlgTitle,mTODOHelpID))
{
    const CallBack chgcb( mCB(this,uiProbDenFuncGen,chgCB) );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, "Number of dimensions");
    nrdimsfld_ = lsb->box();
    nrdimsfld_->setInterval( 1, 3, 1 );
    nrdimsfld_->setValue( 2 );
    nrdimsfld_->valueChanging.notify( chgcb );
    
    gengaussfld_ = new uiGenInput( this, "Distribution type",
	    			   BoolInpSpec(true,"Gaussian","Uniform") );
    gengaussfld_->attach( alignedBelow, lsb );
    gengaussfld_->valuechanged.notify( chgcb );

    uiGroup* alfld = gengaussfld_;
    for ( int idx=0; idx<3; idx++ )
    {
	uiGenInput* nmfld = new uiGenInput( this,
			BufferString("Dimension ",idx+1,": Name") );
	uiGenInput* rgfld = new uiGenInput( this, "Range",
				FloatInpSpec(), FloatInpSpec() );
	nmflds_ += nmfld; rgflds_ += rgfld;
	nmfld->attach( alignedBelow, alfld );
	rgfld->attach( rightOf, nmfld );
	alfld = nmfld;
    }

    lsb = new uiLabeledSpinBox( this, "Size of the dimensions" );
    nrnodesfld_ = lsb->box();
    nrnodesfld_->setInterval( 3, 10000, 1 );
    nrnodesfld_->setValue( 25 );
    lsb->attach( alignedBelow, alfld );

    alfld = lsb;
    for ( int idx=0; idx<3; idx++ )
    {
	uiGenInput* expstdfld = new uiGenInput( this,
				BufferString("Dimension ",idx+1,": Exp/Std"),
				FloatInpSpec(), FloatInpSpec() );
	expstdfld->attach( alignedBelow, alfld );
	expstdflds_ += expstdfld;
	alfld = expstdfld;
    }

    dir01fld_ = new uiGenInput( this, "Angle (deg) Dim 1 -> Dim 2",
				FloatInpSpec(0) );
    dir01fld_->setElemSzPol( uiObject::Small );
    dir01fld_->attach( rightOf, expstdflds_[1] );
    dir02fld_ = new uiGenInput( this, "Dim 1 -> Dim 3", FloatInpSpec(0) );
    dir02fld_->attach( alignedBelow, dir01fld_ );
    dir02fld_->setElemSzPol( uiObject::Small );
    dir12fld_ = new uiGenInput( this, "Dim 2 -> Dim 3", FloatInpSpec(0) );
    dir12fld_->attach( rightOf, dir02fld_ );
    dir12fld_->setElemSzPol( uiObject::Small );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, dir12fld_ );

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt,"Output Probability Density Function");
    outfld_->attach( alignedBelow, alfld );
    outfld_->attach( ensureBelow, sep );

    postFinalise().notify( chgcb );
}

void chgCB( CallBacker* )
{
    const int nrdims = nrdimsfld_->getValue();
    const bool isgauss = gengaussfld_->getBoolValue();
    for ( int idx=0; idx<nmflds_.size(); idx++ )
    {
	const bool havedim = idx < nrdims;
	nmflds_[idx]->display( havedim );
	rgflds_[idx]->display( havedim );
	expstdflds_[idx]->display( havedim && isgauss );
    }
    dir01fld_->display( isgauss && nrdims == 2 );
    dir02fld_->display( false );
    dir12fld_->display( false );
}


MultiID newObjKey() const
{
    const IOObj* ioobj = outfld_->ioobj();
    return ioobj ? ioobj->key() : MultiID();
}

#define mErrRet(s1,s2) { uiMSG().error(s1,s2); return false; }

bool  chkInputFields()
{
    const int nrdims = nrdimsfld_->getValue();
    const bool isgauss = gengaussfld_->getBoolValue();
    for ( int idx=0; idx<nrdims ; idx++  )
    {
	Interval<float> range( rgflds_[idx]->getfValue(0),
			       rgflds_[idx]->getfValue(1) );
	if ( nmflds_[idx]->isUndef() )
		mErrRet( "Please enter a name for Dimension ",
			 toString(idx+1) );
	if ( rgflds_[idx]->isUndef(0) || rgflds_[idx]->isUndef(1) )
		mErrRet( "Please enter a valid range for Dimension ",
			 toString(idx+1) );
	if ( range.start > range.stop )
		mErrRet( "Range start is greater than range stop in Dimension ",
			 toString(idx+1) );
	if ( isgauss )
	{
	    if ( expstdflds_[idx]->isUndef(0)  ) 
		mErrRet( "Please provide an Expected value for Dimension ",
			 toString(idx+1) );
	    if ( expstdflds_[idx]->isUndef(1) )
		mErrRet( "Please provide a Standard Deviation for Dimension ",
			 toString(idx+1) );
	}
    }

    return true;
}

bool acceptOK( CallBacker* )
{
    if ( !chkInputFields() )
	return false;
    readInput();
    switch ( nrdims_ )
    {
	case 1 :
	    {
		savePDF( *calcPDF1D() );
	    }
	    break;
	case 2 :
	    {
		savePDF( *calcPDF2D() );
	    }
	    break;
	case 3 :
	    {
		savePDF( *calcPDF3D() );
	    }
	    break;
    }
    return true;
    
}

    uiSpinBox*			nrdimsfld_;
    uiSpinBox*			nrnodesfld_;
    uiGenInput*			gengaussfld_;
    ObjectSet<uiGenInput>	nmflds_;
    ObjectSet<uiGenInput>	rgflds_;
    ObjectSet<uiGenInput>	expstdflds_;
    uiGenInput*			dir01fld_;
    uiGenInput*			dir02fld_;
    uiGenInput*			dir12fld_;
    uiIOObjSel*			outfld_;

    int				nrdims_;
    TypeSet<float>		sigmas_;
    TypeSet<float>		mus_;
    int				nrnodes_; 
    TypeSet<StepInterval<float> > ranges_;
    bool			isgauss_;

    void			readInput();
    ProbDenFunc*		calcPDF1D() const;
    ProbDenFunc*		calcPDF2D() const;
    ProbDenFunc*		calcPDF3D() const;
    bool			savePDF(const ProbDenFunc&) const;
    float			calcGaussian1D( float x ) const;
    float			calcGaussian2D( float x, float y ) const;
    float			calcGaussian3D( float x, float y, float z ) const;
};


void uiProbDenFuncMan::genPush( CallBacker* )
{
    uiProbDenFuncGen dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.newObjKey() );
}


void uiProbDenFuncMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    txt += getFileInfo();

    mGetPDF(pdf);
    if ( pdf )
    {
	IOPar par;
	pdf->fillPar( par );
	txt += BufferString( "Type: ", pdf->getTypeStr() );
	for ( int idx=0; idx<pdf->nrDims(); idx++ )
	{
	    BufferString lbl( "\nDimension ", idx+1, ": " );
	    txt += lbl; txt += pdf->dimName(idx);
	}
    }
    setInfo( txt );
}



ProbDenFunc* uiProbDenFuncGen::calcPDF1D() const
{
    float val = cMaxProbVal;
    Array1DImpl<float> arr( nrnodes_ );
    float scalefactor = isgauss_ ? cMaxProbVal / calcGaussian1D(mus_[0]) : 1.0f;
    for ( int idx=0; idx<nrnodes_; idx++ )
    {
	const float xval = ranges_[0].start + idx * ranges_[0].step;
	if ( isgauss_ )
	    val = scalefactor * calcGaussian1D( xval );
	    
	arr.set( idx, val );
    }

    Sampled1DProbDenFunc* pdf = new Sampled1DProbDenFunc( arr );
    pdf->sd_.start = ranges_[0].start; pdf->sd_.step = ranges_[0].step;
    pdf->setDimName( 0, nmflds_[0]->text() );
    return pdf ;
}


ProbDenFunc* uiProbDenFuncGen::calcPDF2D() const
{
    const float val = cMaxProbVal;
    Array2DImpl<float> arr( nrnodes_, nrnodes_ );
    if ( !isgauss_ )
	arr.setAll( val );
    else 
    {
	float scalefactor = cMaxProbVal / calcGaussian2D( mus_[0], mus_[1] );
	for ( int idx=0; idx<nrnodes_; idx++ )
	{
	    const float xval = ranges_[0].start + idx * ranges_[0].step;
	    for ( int idy=0; idy<nrnodes_; idy++ )
	    {
		const float yval = ranges_[1].start + idy * ranges_[1].step;
		arr.set( idx, idy, scalefactor * calcGaussian2D(xval,yval) );
	    }
	}  
    }

    Sampled2DProbDenFunc* pdf = new Sampled2DProbDenFunc( arr );
    pdf->sd0_.start = ranges_[0].start; pdf->sd0_.step = ranges_[0].step;
    pdf->sd1_.start = ranges_[1].start; pdf->sd1_.step = ranges_[1].step;
    pdf->setDimName( 0, nmflds_[0]->text() );
    pdf->setDimName( 1, nmflds_[1]->text() );
    return pdf;
}


ProbDenFunc* uiProbDenFuncGen::calcPDF3D() const
{
    const float val = cMaxProbVal;
    Array3DImpl<float> arr( nrnodes_, nrnodes_, nrnodes_ );
    if ( !isgauss_ )
	arr.setAll( val );
    else 
    {
	float scalefactor = cMaxProbVal / calcGaussian3D( mus_[0], mus_[1],	
								    mus_[2] );
	for ( int idx=0; idx<nrnodes_; idx++ )
	{
	    const float xval = ranges_[0].start + idx * ranges_[0].step;
	    for ( int idy=0; idy<nrnodes_; idy++ )
	    {
		const float yval = ranges_[1].start + idy * ranges_[1].step;
		for ( int idz=0; idz<nrnodes_; idz++ )
		{
		    const float zval = ranges_[2].start + idz * ranges_[2].step;
		    arr.set( idx, idy, idz, 
				scalefactor * calcGaussian3D(xval,yval,zval) );
		}
	    }
	}
    }

    SampledNDProbDenFunc* pdf = new SampledNDProbDenFunc( arr );
    pdf->sds_[0].start = ranges_[0].start; pdf->sds_[0].step = ranges_[0].step;
    pdf->sds_[1].start = ranges_[1].start; pdf->sds_[1].step = ranges_[1].step;
    pdf->sds_[2].start = ranges_[2].start; pdf->sds_[2].step = ranges_[2].step;
    pdf->setDimName( 0, nmflds_[0]->text() );
    pdf->setDimName( 1, nmflds_[1]->text() );
    pdf->setDimName( 2, nmflds_[2]->text() );
    return pdf;
}


float uiProbDenFuncGen::calcGaussian1D( float x ) const
{
    float res = exp( - ( pow(x-mus_[0],2) / (2*pow(sigmas_[0],2)) ) )
		/ ( sqrt(2*3.14)*pow (sigmas_[0],2) );
    return res;   
}


float uiProbDenFuncGen::calcGaussian2D( float x, float y ) const
{
    float ang01 = 0;
    if ( !dir01fld_->isUndef() )
	ang01 = dir01fld_->getfValue()*3.14/180;

    float val1 = (  ((cos(ang01)*cos(ang01))/ (2*sigmas_[0]*sigmas_[0])) +
                ((sin(ang01)*sin(ang01))/(2*sigmas_[1]*sigmas_[1])) );

    float val2 = ( -(sin(2*ang01)/(4*sigmas_[0]*sigmas_[0])) + 
		    ( sin(2*ang01)/(4*sigmas_[1]*sigmas_[1])) );

    float val3 = ( ((cos(ang01)*cos(ang01))/ (2*sigmas_[1]*sigmas_[1])) +
                 ((sin(ang01)*sin(ang01))/(2*sigmas_[0]*sigmas_[0])) );

    return exp( -(pow(val1*(x-mus_[0]),2) + 2*val2*(x-mus_[0])*(y-mus_[1])
					    + pow(val3*(y-mus_[1]),2)) );
}


float uiProbDenFuncGen::calcGaussian3D( float x, float y, float z ) const
{
    float val1 = pow( x-mus_[0], 2 ) / ( 2*sigmas_[0]*sigmas_[0] ); 
    float val2 = pow( y-mus_[1], 2 ) / ( 2*sigmas_[1]*sigmas_[1] ); 
    float val3 = pow( z-mus_[2], 2 ) / ( 2*sigmas_[2]*sigmas_[2] ); 
    return exp( -(val1+val2+val3) );
}


bool uiProbDenFuncGen::savePDF( const ProbDenFunc& pdf ) const
{
    const IOObj* pdfioobj = outfld_->ioobj();
    ProbDenFuncTranslator::write( pdf, *pdfioobj );
    return true;
}


void uiProbDenFuncGen::readInput()
{
    nrdims_ = nrdimsfld_->getValue();
    nrnodes_= nrnodesfld_->getValue();
    isgauss_= gengaussfld_->getBoolValue();
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	Interval<float> range( rgflds_[idx]->getfValue(0),
			       rgflds_[idx]->getfValue(1) );
	ranges_ += StepInterval<float>( range.start, range.stop, 
				      range.width() / (nrnodes_-1) );
	mus_ += expstdflds_[idx]->getfValue(0);
	sigmas_ += expstdflds_[idx]->getfValue(1);
    }
}


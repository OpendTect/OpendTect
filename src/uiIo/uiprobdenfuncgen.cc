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
#include "uichecklist.h"
#include "uiioobjsel.h"
#include "uiseparator.h"
#include "uimsg.h"

#include "sampledprobdenfunc.h"
#include "gaussianprobdenfunc.h"
#include "probdenfunctr.h"

static const float sqrt2pi = Math::Sqrt(2*M_PIf);
static const float cMaxProbVal = 100.0f;


class uiProbDenFuncGenSampled : public uiDialog
{
public:

			uiProbDenFuncGenSampled(uiParent*,int nrdim,bool gauss,
						MultiID&);

    uiSpinBox*		nrnodesfld_;
    ObjectSet<uiGenInput> nmflds_;
    ObjectSet<uiGenInput> rgflds_;
    ObjectSet<uiGenInput> expstdflds_;
    uiGenInput*		dir01fld_;
    uiGenInput*		dir02fld_;
    uiGenInput*		dir12fld_;
    uiIOObjSel*		outfld_;

    const int		nrdims_;
    TypeSet<float>	sigmas_;
    TypeSet<float>	mus_;
    int			nrnodes_;
    TypeSet<StepInterval<float> > ranges_;
    MultiID&		ioobjky_;

    inline bool		isGauss() const	{ return !expstdflds_.isEmpty(); }
    bool		chkInputFields();
    void		readInput();
    ProbDenFunc*	calcPDF1D() const;
    ProbDenFunc*	calcPDF2D() const;
    ProbDenFunc*	calcPDF3D() const;
    bool		savePDF(const ProbDenFunc&) const;
    float		calcGaussian1D(float) const;
    float		calcGaussian2D(float,float) const;
    float		calcGaussian3D(float,float,float) const;

    bool		acceptOK(CallBacker*);
    void		chgCB(CallBacker*);
};


class uiProbDenFuncGenGaussian : public uiDialog
{
public:

			uiProbDenFuncGenGaussian(uiParent*,int nrdim,MultiID&);

    const int		nrdims_;
    MultiID&		ioobjky_;

    uiTabStack*		tabstack_;

    bool		acceptOK(CallBacker*);

};


uiProbDenFuncGen::uiProbDenFuncGen( uiParent* p )
    : uiDialog(p,Setup("Generate a PDF",mNoDlgTitle,"112.1.2"))
{

    choicefld_ = new uiCheckList( this, uiCheckList::OneOnly );
    choicefld_->addItem( "Create an &empty PDF to edit by hand" );
    choicefld_->addItem( "Create an editable PDF &filled with Gaussian values");
    choicefld_->addItem( "Create a full &Gaussian PDF" );
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
    const bool isfullgauss = choice == 2;
    nrdimfld_->setInterval( 1, isfullgauss ? 20 : 3, 1 );
}


bool uiProbDenFuncGen::acceptOK( CallBacker* )
{
    const int choice = choicefld_->firstChecked();
    const int nrdims = nrdimfld_->getValue();

    if ( choice == 2 )
    {
	uiProbDenFuncGenGaussian dlg( this, nrdims, ioobjky_ );
	return dlg.go();
    }

    uiProbDenFuncGenSampled dlg( this, nrdims, choice==1, ioobjky_ );
    return dlg.go();
}



uiProbDenFuncGenSampled::uiProbDenFuncGenSampled( uiParent* p, int nrdim,
						  bool isgauss, MultiID& ky )
    : uiDialog(p,Setup("Generate editable PDF",mNoDlgTitle,mTODOHelpKey))
    , nrdims_(nrdim)
    , ioobjky_(ky)
    , dir01fld_(0)
    , dir02fld_(0)
    , dir12fld_(0)
{
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	uiGenInput* nmfld = new uiGenInput( this, nrdims_ == 1 ?
		"Variable name" : BufferString("Dimension ",idx+1,": Name") );
	uiGenInput* rgfld = new uiGenInput( this, "Value range",
				FloatInpSpec(), FloatInpSpec() );
	nmflds_ += nmfld; rgflds_ += rgfld;
	if ( idx )
	    nmfld->attach( alignedBelow, nmflds_[idx-1] );
	rgfld->attach( rightOf, nmfld );
    }

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
	    nrdims_ == 1 ? "Number of cells" : "Number of cells per dimension");
    nrnodesfld_ = lsb->box();
    nrnodesfld_->setInterval( 3, 10000, 1 );
    nrnodesfld_->setValue( 25 );
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

	if ( nrdims_ > 1 )
	{
	    dir01fld_ = new uiGenInput( this, "Angle (deg) Dim 1 -> Dim 2",
					FloatInpSpec(0) );
	    dir01fld_->setElemSzPol( uiObject::Small );
	    dir01fld_->attach( rightOf, expstdflds_[1] );
	}
	if ( nrdims_ == 3 )
	{
	    dir02fld_ = new uiGenInput( this, "Angle Dim 1 -> 3",
					FloatInpSpec(0) );
	    dir02fld_->attach( alignedBelow, dir01fld_ );
	    dir02fld_->setElemSzPol( uiObject::Small );
	    dir12fld_ = new uiGenInput( this, "Angle Dim 2 -> 3",
						FloatInpSpec(0) );
	    dir12fld_->attach( rightOf, dir02fld_ );
	    dir12fld_->setElemSzPol( uiObject::Small );
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


#define mErrRet(s1,s2) { uiMSG().error(s1,s2); return false; }

bool uiProbDenFuncGenSampled::chkInputFields()
{
    for ( int idx=0; idx<nrdims_ ; idx++  )
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
	if ( isGauss() )
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


bool uiProbDenFuncGenSampled::acceptOK( CallBacker* )
{
    if ( !chkInputFields() )
	return false;
    readInput();
    bool ret = false;
    switch ( nrdims_ )
    {
	case 1 : ret = savePDF( *calcPDF1D() ); break;
	case 2 : ret = savePDF( *calcPDF2D() ); break;
	case 3 : ret = savePDF( *calcPDF3D() ); break;
    }

    return ret;
}


ProbDenFunc* uiProbDenFuncGenSampled::calcPDF1D() const
{
    float val = cMaxProbVal;
    Array1DImpl<float> arr( nrnodes_ );
    if ( !isGauss() )
	arr.setAll( val );
    else
    {
	const float scalefactor = cMaxProbVal / calcGaussian1D(mus_[0]);
	for ( int idx=0; idx<nrnodes_; idx++ )
	{
	    const float xval = ranges_[0].start + idx * ranges_[0].step;
	    arr.set( idx, scalefactor * calcGaussian1D(xval) );
	}
    }

    Sampled1DProbDenFunc* pdf = new Sampled1DProbDenFunc( arr );
    pdf->sd_.start = ranges_[0].start; pdf->sd_.step = ranges_[0].step;
    pdf->setDimName( 0, nmflds_[0]->text() );
    return pdf ;
}


ProbDenFunc* uiProbDenFuncGenSampled::calcPDF2D() const
{
    const float val = cMaxProbVal;
    Array2DImpl<float> arr( nrnodes_, nrnodes_ );
    if ( !isGauss() )
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


ProbDenFunc* uiProbDenFuncGenSampled::calcPDF3D() const
{
    const float val = cMaxProbVal;
    Array3DImpl<float> arr( nrnodes_, nrnodes_, nrnodes_ );
    if ( !isGauss() )
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


float uiProbDenFuncGenSampled::calcGaussian1D( float x ) const
{
    float res = exp( - ( pow(x-mus_[0],2) / (2*pow(sigmas_[0],2)) ) )
		/ ( sqrt2pi*pow (sigmas_[0],2) );
    return res;
}


float uiProbDenFuncGenSampled::calcGaussian2D( float x, float y ) const
{
    float ang01 = 0;
    if ( !dir01fld_->isUndef() )
	ang01 = dir01fld_->getfValue()*3.14f/180;

    float val1 = (  ((cos(ang01)*cos(ang01))/ (2*sigmas_[0]*sigmas_[0])) +
                ((sin(ang01)*sin(ang01))/(2*sigmas_[1]*sigmas_[1])) );

    float val2 = ( -(sin(2*ang01)/(4*sigmas_[0]*sigmas_[0])) +
		    ( sin(2*ang01)/(4*sigmas_[1]*sigmas_[1])) );

    float val3 = ( ((cos(ang01)*cos(ang01))/ (2*sigmas_[1]*sigmas_[1])) +
                 ((sin(ang01)*sin(ang01))/(2*sigmas_[0]*sigmas_[0])) );

    return exp( -(pow(val1*(x-mus_[0]),2) + 2*val2*(x-mus_[0])*(y-mus_[1])
					    + pow(val3*(y-mus_[1]),2)) );
}


float uiProbDenFuncGenSampled::calcGaussian3D( float x, float y, float z ) const
{
    float val1 = pow( x-mus_[0], 2 ) / ( 2*sigmas_[0]*sigmas_[0] );
    float val2 = pow( y-mus_[1], 2 ) / ( 2*sigmas_[1]*sigmas_[1] );
    float val3 = pow( z-mus_[2], 2 ) / ( 2*sigmas_[2]*sigmas_[2] );
    return exp( -(val1+val2+val3) );
}


bool uiProbDenFuncGenSampled::savePDF( const ProbDenFunc& pdf ) const
{
    const IOObj* pdfioobj = outfld_->ioobj();
    if ( !pdfioobj )
	return false;

    ioobjky_ = pdfioobj->key();
    if ( !ProbDenFuncTranslator::write(pdf,*pdfioobj) )
    {
	uiMSG().error( "Could not write PDF to disk" );
	return false;
    }

    return true;
}


void uiProbDenFuncGenSampled::readInput()
{
    nrnodes_= nrnodesfld_->getValue();
    for ( int idx=0; idx<nrdims_; idx++ )
    {
	Interval<float> range( rgflds_[idx]->getfValue(0),
			       rgflds_[idx]->getfValue(1) );
	ranges_ += StepInterval<float>( range.start, range.stop,
				      range.width() / (nrnodes_-1) );
	if ( isGauss() )
	{
	    mus_ += expstdflds_[idx]->getfValue(0);
	    sigmas_ += expstdflds_[idx]->getfValue(1);
	}
    }
}


#include "uilabel.h" // for TODO

uiProbDenFuncGenGaussian::uiProbDenFuncGenGaussian( uiParent* p, int nrdim,
						    MultiID& ky )
    : uiDialog(p,Setup("Generate Gaussian PDF",mNoDlgTitle,mTODOHelpKey))
    , nrdims_(nrdim)
    , ioobjky_(ky)
{
    new uiLabel( this, "TODO: implement" );
}


bool uiProbDenFuncGenGaussian::acceptOK( CallBacker* )
{
    return true;
}

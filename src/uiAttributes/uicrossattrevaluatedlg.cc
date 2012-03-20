/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          March 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uicrossattrevaluatedlg.cc,v 1.2 2012-03-20 21:48:31 cvsyuancheng Exp $";

#include "uicrossattrevaluatedlg.h"
#include "uievaluatedlg.h"
#include "uigeninput.h"
#include "datainpspec.h"
#include "iopar.h"
#include "uislider.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uiattrdesced.h"
#include "uiattrdescseted.h"
#include "attribdescsetman.h"

#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "odver.h"
#include "survinfo.h"

using namespace Attrib;

static const StepInterval<int> cSliceIntv(2,30,1);

uiCrossAttrEvaluateDlg::uiCrossAttrEvaluateDlg( uiParent* p, 
	uiAttribDescSetEd& uads, bool store ) 
    : uiDialog(p,uiDialog::Setup("Cross attributes evaluation","Settings",
		"101.3.1").modal(false).oktext("Accept").canceltext(""))
    , calccb(this)
    , showslicecb(this)
    , initpar_(*new IOPar)
    , enabstore_(store)
    , haspars_(false)
    , attrset_(*uads.getSet())  
{
    attrset_.fillPar( initpar_ );

    uiGroup* pargrp = new uiGroup( this, "" );
    pargrp->setStretch( 1, 1 );

    BufferStringSet paramnms;
    uads.getUiAttribParamGrps( true, pargrp, grps_, paramnms, userattnms_ );
    if ( grps_.isEmpty() ) return;
    
    haspars_ = true;

    uiGroup* grp = new uiGroup( this, "Attr-Params" );
    uiLabel* paramlabel = new uiLabel( grp, "Evaluate parameters" );
    paramsfld_ = new uiListBox( grp );
    paramsfld_->attach( ensureBelow, paramlabel );
    paramsfld_->addItems( paramnms );
    paramsfld_->selectionChanged.notify(
	    mCB(this,uiCrossAttrEvaluateDlg,variableSel));

    uiLabel* attrlabel = new uiLabel( grp, "Attributes" );
    attrnmsfld_ = new uiListBox( grp, "From attributes", true );
    attrnmsfld_->attach( rightOf, paramsfld_ );
    attrlabel->attach( alignedAbove, attrnmsfld_ );
    attrlabel->attach( rightTo, paramlabel );

    pargrp->attach( alignedBelow, grp );
    pargrp->setHAlignObj( grps_[0] );

    nrstepsfld = new uiLabeledSpinBox( this, "Nr of steps" );
    nrstepsfld->box()->setInterval( cSliceIntv );
    nrstepsfld->attach( alignedBelow, pargrp );

    calcbut = new uiPushButton( this, "Calculate", true );
    calcbut->activated.notify( mCB(this,uiCrossAttrEvaluateDlg,calcPush) );
    calcbut->attach( rightTo, nrstepsfld );

    sliderfld = new uiSliderExtra( this, "Slice", "Slice slider" );
    sliderfld->attach( alignedBelow, nrstepsfld );
    sliderfld->sldr()->valueChanged.notify( 
	    mCB(this,uiCrossAttrEvaluateDlg,sliderMove) );
    sliderfld->sldr()->setTickMarks( uiSlider::Below );
    sliderfld->setSensitive( false );

    storefld = new uiCheckBox( this, "Store slices on 'Accept'" );
    storefld->attach( alignedBelow, sliderfld );
    storefld->setChecked( false );
    storefld->setSensitive( false );

    displaylbl = new uiLabel( this, "" );
    displaylbl->attach( widthSameAs, sliderfld );
    displaylbl->attach( alignedBelow, storefld );

    postFinalise().notify( mCB(this,uiCrossAttrEvaluateDlg,doFinalise) );
}


void uiCrossAttrEvaluateDlg::doFinalise( CallBacker* )
{
    variableSel(0);
}


uiCrossAttrEvaluateDlg::~uiCrossAttrEvaluateDlg()
{
    deepErase( lbls_ );
}


void uiCrossAttrEvaluateDlg::variableSel( CallBacker* )
{
    const int sel = paramsfld_->currentItem();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx==sel );

    attrnmsfld_->setEmpty();
    attrnmsfld_->addItems( userattnms_[sel] );
}


void uiCrossAttrEvaluateDlg::calcPush( CallBacker* )
{
    float vsn = mODMajorVersion + 0.1*mODMinorVersion;
    attrset_.usePar( initpar_, vsn );
    sliderfld->sldr()->setValue(0);
    lbls_.erase();
    specs_.erase();

    const int sel = paramsfld_->currentItem();
    if ( sel >= grps_.size() ) return;
    AttribParamGroup* pargrp = grps_[sel];

    Desc& srcad = *attrset_.desc( 0 );
    BufferString userchosenref = srcad.userRef();
    const int nrsteps = nrstepsfld->box()->getValue();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	Desc* newad = idx ? new Desc(srcad) : &srcad;
	if ( !newad ) return;
	pargrp->updatePars( *newad, idx );
	pargrp->updateDesc( *newad, idx );

	BufferString defstr; newad->getDefStr( defstr ); //Only for debugging

	const char* lbl = pargrp->getLabel();
	BufferString usrref = newad->attribName();
	usrref += " - "; usrref += lbl;
	newad->setUserRef( usrref );
	if ( newad->selectedOutput()>=newad->nrOutputs() )
	{
	    newad->ref(); newad->unRef();
	    continue;
	}

	if ( idx ) 
	    attrset_.addDesc( newad );

	lbls_ += new BufferString( lbl );
	SelSpec as; as.set( *newad );

	// trick : use as -> objectRef to store the userref;
	// possible since objref_ is only used for NN which cannot be evaluated
	as.setObjectRef( userchosenref.buf() );
		
	specs_ += as;
    }

    if ( specs_.isEmpty() )
	return;

    calccb.trigger();

    if ( enabstore_ ) storefld->setSensitive( true );
    sliderfld->setSensitive( true );
    sliderfld->sldr()->setMaxValue( nrsteps-1 );
    sliderfld->sldr()->setTickStep( 1 );
    sliderMove(0);
}


void uiCrossAttrEvaluateDlg::sliderMove( CallBacker* )
{
    const int sliceidx = sliderfld->sldr()->getIntValue();
    if ( sliceidx >= lbls_.size() )
	return;

    displaylbl->setText( lbls_[sliceidx]->buf() );
    showslicecb.trigger( sliceidx );
}


bool uiCrossAttrEvaluateDlg::acceptOK( CallBacker* )
{
    const int sliceidx = sliderfld->sldr()->getIntValue();
    //if ( sliceidx < specs_.size() )
	//seldesc_ = attrset_.getDesc( specs_[sliceidx].id() );

    return true;
}


void uiCrossAttrEvaluateDlg::getEvalSpecs( TypeSet<Attrib::SelSpec>& specs ) const
{
    specs = specs_;
}


bool uiCrossAttrEvaluateDlg::storeSlices() const
{
    return enabstore_ ? storefld->isChecked() : false;
}

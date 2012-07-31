/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          March 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uicrossattrevaluatedlg.cc,v 1.14 2012-07-31 08:52:30 cvsbert Exp $";

#include "uicrossattrevaluatedlg.h"

#include "attribdesc.h"
#include "attribsel.h"
#include "envvars.h"
#include "filepath.h"
#include "keystrs.h"
#include "odver.h"

#include "uiattrdescseted.h"
#include "uibutton.h"
#include "uievaluatedlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uislider.h"
#include "uispinbox.h"

using namespace Attrib;

static const StepInterval<int> cSliceIntv(2,30,1);

uiCrossAttrEvaluateDlg::uiCrossAttrEvaluateDlg( uiParent* p, 
	uiAttribDescSetEd& uads, bool store ) 
    : uiDialog(p,uiDialog::Setup("Cross attributes evaluation","Settings",
		"101.3.1").modal(false).oktext("Accept").canceltext(""))
    , calccb(this)
    , showslicecb(this)
    , initpar_(*new IOPar)
    , seldesc_(0)  
    , enabstore_(store)
    , haspars_(false)
    , attrset_(*new DescSet(*uads.getSet()))
    , paramsfld_(0)
    , srcid_(-1,true)  
{
    if ( !uads.curDesc() )
	return;

    srcid_ = uads.curDesc()->id();
    attrset_.fillPar( initpar_ );

    uiGroup* pargrp = new uiGroup( this, "" );
    pargrp->setStretch( 1, 1 );

    BufferStringSet paramnms;
    uads.getUiAttribParamGrps( pargrp, grps_, paramnms, userattnms_ );
    if ( grps_.isEmpty() ) return;
    
    haspars_ = true;

    uiGroup* grp = new uiGroup( this, "Attr-Params" );
    uiLabel* paramlabel = new uiLabel( grp, "Evaluate parameters" );
    paramsfld_ = new uiListBox( grp );
    paramsfld_->attach( ensureBelow, paramlabel );
    paramsfld_->addItems( paramnms );
    paramsfld_->selectionChanged.notify(
	    mCB(this,uiCrossAttrEvaluateDlg,parameterSel));

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
    parameterSel(0);
}


uiCrossAttrEvaluateDlg::~uiCrossAttrEvaluateDlg()
{
    delete &attrset_;
    deepErase( lbls_ );
}


void uiCrossAttrEvaluateDlg::parameterSel( CallBacker* )
{
    const int sel = paramsfld_->currentItem();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx==sel );

    attrnmsfld_->setEmpty();
    attrnmsfld_->addItems( userattnms_[sel] );
}


#define mSetSelSpecAndLbl( origad ) \
    Desc* newad = new Desc(origad); \
    if ( !newad ) return;	\
    pargrp->updatePars( *newad, idx ); \
    pargrp->updateDesc( *newad, idx ); \
    const char* lbl = pargrp->getLabel(); \
    if ( !lbladded ) \
    { \
	lbladded = true; \
	lbls_ += new BufferString(lbl); \
	BufferString defstr; \
	newad->getDefStr( defstr ); \
	defstr_.add( defstr ); \
    } \
    BufferString usrref = newad->attribName(); \
    usrref += " - "; usrref += lbl; \
    newad->setUserRef( usrref ); \
    attrset_.addDesc( newad ); \
    SelSpec as; \
    as.set( *newad ); \
    as.setObjectRef( srcad.userRef() ) 


void uiCrossAttrEvaluateDlg::calcPush( CallBacker* )
{
    if ( !attrset_.getDesc(srcid_) )
	return;

    float vsn = mODMajorVersion + 0.1*mODMinorVersion;
    attrset_.usePar( initpar_, vsn );
    sliderfld->sldr()->setValue(0);
    lbls_.erase();
    specs_.erase();
    srcspecids_.erase();
    defstr_.erase();

    const int sel = paramsfld_->currentItem();
    if ( sel<0 || sel>=grps_.size() ) return;
    AttribParamGroup* pargrp = grps_[sel];

    TypeSet<TypeSet<DescID> > ancestorids;//for each selected
    TypeSet<TypeSet<int> > aids;//index of children
    getSelDescIDs( ancestorids, aids );
  
    Desc& srcad = *attrset_.getDesc( srcid_ );
    const int selsz = seldeschildids_.size();
    const int nrsteps = nrstepsfld->box()->getValue();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	bool lbladded = false;
	TypeSet<int> startidx;
	for ( int ci=0; ci<selsz; ci++ )
	{
	    startidx += attrset_.size();
	    /*add me and my ancestors except the top one*/
	    for ( int pi=0; pi<ancestorids[ci].size(); pi++ )
	    {
	        Desc* ds = !pi ? attrset_.getDesc(seldeschildids_[ci]) :
		    		 attrset_.getDesc(ancestorids[ci][pi-1] );
		if ( !ds ) return;

		mSetSelSpecAndLbl(*ds);
	    }
	}

	mSetSelSpecAndLbl(srcad);
	srcspecids_ += newad->id();
	specs_ += as; 

	/*reset dependency*/
	for ( int ci=0; ci<selsz; ci++ )
	{
	    int didx = startidx[ci];
	    const int lastpidx = aids[ci].size()-1;
	    for ( int pi=0; pi<lastpidx; pi++ )
	    {
		attrset_.desc(didx+1)->setInput( aids[ci][pi], 
			attrset_.desc(didx));
		didx++;
	    }
	    newad->setInput( aids[ci][lastpidx], attrset_.desc(didx) );
	}
    }
    if ( specs_.isEmpty() )
	return;

    // For debugging
    if ( GetEnvVarYN("OD_WRITE_EVALUATE_ATTRIBUTE_SET") )
    {
	IOPar newpar;
	attrset_.fillPar( newpar );
	for ( int idx=0; idx<srcspecids_.size(); idx++ )
	    newpar.set( IOPar::compKey(sKey::Output(),idx),
		        srcspecids_[idx].asInt() );

	newpar.write( FilePath::getTempName("par"), sKey::Attributes() );
    }

    calccb.trigger();

    if ( enabstore_ ) storefld->setSensitive( true );
    sliderfld->setSensitive( true );
    sliderfld->sldr()->setMaxValue( nrsteps-1 );
    sliderfld->sldr()->setTickStep( 1 );
    sliderMove(0);
}


void uiCrossAttrEvaluateDlg::getSelDescIDs(
	TypeSet<TypeSet<DescID> >& ancestorids, TypeSet<TypeSet<int> >& aids ) 
{
    seldeschildids_.erase();
    
    TypeSet<int> attrselected;
    attrnmsfld_->getSelectedItems( attrselected );
    
    Desc& srcad = *attrset_.getDesc( srcid_ );
    const int sel = paramsfld_->currentItem();
    for ( int idx=0; idx<attrselected.size(); idx++ )
    {
	const char* userattnm = userattnms_[sel].get(attrselected[idx]).buf();
	for ( int idy=0; idy<attrset_.size(); idy++ )
	{
	    if ( !attrset_.desc(idy) || attrset_.desc(idy)==&srcad ) 
		continue;

	    BufferString anm = attrset_.desc(idy)->userRef();
	    if ( !anm.isEqual(userattnm,true) ) 
		continue;

	    seldeschildids_ += attrset_.desc(idy)->id();
	    break;
	}
    }

    const int selsz = seldeschildids_.size();
    for ( int idx=0; idx<selsz; idx++ )
    {
	TypeSet<DescID> tmpids;
	TypeSet<int> ids;
	srcad.getAncestorIDs( seldeschildids_[idx], tmpids, ids );
	ancestorids += tmpids;
	aids += ids;
    }
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
    if ( !paramsfld_ || srcspecids_.isEmpty() )
	return true;

    const int sliceidx = sliderfld->sldr()->getIntValue();
    seldesc_ = attrset_.getDesc( srcspecids_[sliceidx] );

    return true;
}


BufferString uiCrossAttrEvaluateDlg::acceptedDefStr() const 
{
    const int sliceidx = sliderfld->sldr()->getIntValue();
    return defstr_.get(sliceidx);
}


void uiCrossAttrEvaluateDlg::getEvalSpecs(
	TypeSet<Attrib::SelSpec>& specs ) const
{ specs = specs_; }


bool uiCrossAttrEvaluateDlg::storeSlices() const
{
    return enabstore_ ? storefld->isChecked() : false;
}

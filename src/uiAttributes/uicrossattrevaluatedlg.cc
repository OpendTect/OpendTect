/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicrossattrevaluatedlg.h"

#include "attribdesc.h"
#include "attribsel.h"
#include "envvars.h"
#include "filepath.h"
#include "keystrs.h"

#include "uiattrdescseted.h"
#include "uibutton.h"
#include "uievaluatedlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uislider.h"
#include "uispinbox.h"
#include "od_helpids.h"

using namespace Attrib;

static const StepInterval<int> cSliceIntv(2,30,1);

uiCrossAttrEvaluateDlg::uiCrossAttrEvaluateDlg( uiParent* p,
	uiAttribDescSetEd& uads, bool store )
    : uiDialog(p,uiDialog::Setup(tr("Cross attribute parameter evaluation"),
				mNoDlgTitle, mODHelpKey(mEvaluateDlgHelpID) )
				.modal(false).oktext(tr("Accept"))
				.canceltext(uiString::emptyString()))
    , calccb(this)
    , showslicecb(this)
    , srcid_(-1,true)
    , attrset_(*new DescSet(*uads.getSet()))
    , initpar_(*new IOPar)
    , enabstore_(store)
    , haspars_(false)
{
    if ( !uads.curDesc() )
	return;

    srcid_ = uads.curDesc()->id();
    attrset_.fillPar( initpar_ );

    uiGroup* pargrp = new uiGroup( this, "" );
    pargrp->setStretch( 1, 1 );

    BufferStringSet paramnms;
    uads.getUiAttribParamGrps( pargrp, grps_, paramnms, userattnms_ );
    if ( grps_.isEmpty() )
	return;

    haspars_ = true;

    uiGroup* grp = new uiGroup( this, "Attr-Params" );
    uiLabel* paramlabel = new uiLabel( grp, tr("Parameter to evaluate") );
    paramsfld_ = new uiListBox( grp );
    paramsfld_->attach( ensureBelow, paramlabel );
    paramsfld_->addItems( paramnms );
    paramsfld_->setStretch( 0, 2 );
    paramsfld_->selectionChanged.notify(
	    mCB(this,uiCrossAttrEvaluateDlg,parameterSel));

    uiLabel* attrlabel = new uiLabel( grp, uiStrings::sAttribute(mPlural) );
    attrnmsfld_ = new uiListBox( grp, "From attributes", OD::ChooseAtLeastOne );
    attrnmsfld_->checkGroup()->display( false, true );
    attrnmsfld_->attach( rightOf, paramsfld_ );
    attrlabel->attach( alignedAbove, attrnmsfld_ );
    attrlabel->attach( rightTo, paramlabel );

    pargrp->attach( alignedBelow, grp );
    pargrp->setHAlignObj( grps_[0] );

    nrstepsfld_ = new uiLabeledSpinBox( this, tr("Nr of steps") );
    nrstepsfld_->box()->setInterval( cSliceIntv );
    nrstepsfld_->attach( alignedBelow, pargrp );

    calcbut_ = new uiPushButton( this, uiStrings::sCalculate(), true );
    calcbut_->activated.notify( mCB(this,uiCrossAttrEvaluateDlg,calcPush) );
    calcbut_->attach( rightTo, nrstepsfld_ );

    sliderfld_ = new uiSlider( this, uiSlider::Setup(tr("Slice")),
			      "Slice slider" );
    sliderfld_->attach( alignedBelow, nrstepsfld_ );
    sliderfld_->valueChanged.notify(
	    mCB(this,uiCrossAttrEvaluateDlg,sliderMove) );
    sliderfld_->setTickMarks( uiSlider::Below );
    sliderfld_->setSensitive( false );

    storefld_ = new uiCheckBox( this, tr("Store slices on 'Accept'") );
    storefld_->attach( alignedBelow, sliderfld_ );
    storefld_->setChecked( false );
    storefld_->setSensitive( false );

    displaylbl_ = new uiLabel( this, uiString::emptyString() );
    displaylbl_->attach( widthSameAs, sliderfld_ );
    displaylbl_->attach( alignedBelow, storefld_ );

    postFinalize().notify( mCB(this,uiCrossAttrEvaluateDlg,doFinalize) );
}


void uiCrossAttrEvaluateDlg::doFinalize( CallBacker* )
{
    parameterSel( nullptr );
    attrnmsfld_->resizeWidthToContents();
}


uiCrossAttrEvaluateDlg::~uiCrossAttrEvaluateDlg()
{
    delete &attrset_;
    delete &initpar_;
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

    attrset_.usePar( initpar_ );
    sliderfld_->setValue(0);
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
    const int nrsteps = nrstepsfld_->box()->getIntValue();
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

	newpar.write( FilePath::getTempFullPath("eval_attrs",
				sParFileExtension()), sKey::Attributes() );
    }

    calccb.trigger();

    if ( enabstore_ ) storefld_->setSensitive( true );
    sliderfld_->setSensitive( true );
    sliderfld_->setMaxValue( mCast(float,nrsteps-1) );
    sliderfld_->setTickStep( 1 );
    sliderMove(0);
}


void uiCrossAttrEvaluateDlg::getSelDescIDs(
	TypeSet<TypeSet<DescID> >& ancestorids, TypeSet<TypeSet<int> >& aids )
{
    seldeschildids_.erase();

    TypeSet<int> attrselected;
    attrnmsfld_->getChosen( attrselected );

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
	    if ( !anm.isEqual(userattnm,OD::CaseInsensitive) )
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
    const int sliceidx = sliderfld_->getIntValue();
    if ( sliceidx >= lbls_.size() )
	return;

    displaylbl_->setText( toUiString(lbls_[sliceidx]->buf()) );
    showslicecb.trigger( sliceidx );
}


bool uiCrossAttrEvaluateDlg::acceptOK( CallBacker* )
{
    if ( !sliderfld_ || srcspecids_.isEmpty() )
	return true;

    const int sliceidx = sliderfld_->getIntValue();
    seldesc_ = attrset_.getDesc( srcspecids_[sliceidx] );

    return true;
}


BufferString uiCrossAttrEvaluateDlg::acceptedDefStr() const
{
    const int sliceidx = sliderfld_->getIntValue();
    return defstr_.get(sliceidx);
}


void uiCrossAttrEvaluateDlg::getEvalSpecs(
	TypeSet<Attrib::SelSpec>& specs ) const
{
    specs = specs_;
}


bool uiCrossAttrEvaluateDlg::storeSlices() const
{
    return enabstore_ && storefld_ ? storefld_->isChecked() : false;
}

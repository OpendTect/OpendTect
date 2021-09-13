/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/

#include "uiprobdenfuncman.h"

#include "uiprobdenfuncgen.h"
#include "uieditpdf.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uiioobjseldlg.h"
#include "uiioobjmanip.h"
#include "uimsg.h"

#include "bufstring.h"
#include "gaussianprobdenfunc.h"
#include "od_helpids.h"
#include "probdenfunctr.h"
#include "sampledprobdenfunc.h"
#include "unitofmeasure.h"

static const int cPrefWidth = 75;

uiProbDenFuncMan::uiProbDenFuncMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(
		uiStrings::phrManage( uiStrings::sProbDensFunc(false,mPlural)),
		mNoDlgTitle,mODHelpKey(mProbDenFuncManHelpID))
			.nrstatusflds(1).modal(false),
		   ProbDenFuncTranslatorGroup::ioContext())
{
    createDefaultUI();
    mAttachCB( selgrp_->getListField()->doubleClicked,
	       uiProbDenFuncMan::browsePush );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "editprdf",
			 tr("Browse/edit this Probability Density Function"),
			 mCB(this,uiProbDenFuncMan,browsePush) );
    manipgrp->addButton( "genprdf",
			 tr("Generate Probability Density Function"),
			 mCB(this,uiProbDenFuncMan,genPush) );

    selgrp_->setPrefWidthInChar( mCast(float,cPrefWidth) );
    infofld_->setPrefWidthInChar( cPrefWidth );
}



uiProbDenFuncMan::~uiProbDenFuncMan()
{
    detachAllNotifiers();
}


void uiProbDenFuncMan::initDlg()
{
    selChg( this );
}


#define mGetPDF(pdf) \
    PtrMan<ProbDenFunc> pdf = ProbDenFuncTranslator::read( *curioobj_ )


void uiProbDenFuncMan::browsePush( CallBacker* )
{
    if ( !curioobj_ ) return;
    mGetPDF(pdf);
    if ( !pdf ) return;

    uiEditProbDenFuncDlg dlg( this, *pdf, true );
    if ( dlg.go() && dlg.isChanged() )
    {
	const int choice = uiMSG().question( tr("PDF changed. Save?"),
		uiStrings::sYes(), m3Dots(tr("As new")), uiStrings::sNo() );
	if ( choice < 0 ) return;

	PtrMan<IOObj> saveioobj = curioobj_->clone();
	if ( choice == 0 )
	{
	    CtxtIOObj ctio( ctxt_ );
	    ctio.ctxt_.forread_ = false;
	    uiIOObjSelDlg seldlg( this, ctio, uiStrings::sSaveAs() );
	    if ( !seldlg.go() || !seldlg.ioObj() ) return;
	    saveioobj = seldlg.ioObj()->clone();
	}

	uiString emsg;
	if ( !ProbDenFuncTranslator::write(*pdf,*saveioobj,&emsg) )
	    uiMSG().error( emsg );
	else
	    selgrp_->fullUpdate( saveioobj->key() );
    }
}


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
    if ( !pdf )
    {
	setInfo( txt );
	return;
    }

    txt.add( "\nType: " ).add( pdf->getTypeStr() );
    for ( int idx=0; idx<pdf->nrDims(); idx++ )
    {
	BufferString lbl( "\nDimension ", idx+1, ": " );
	lbl.add( pdf->dimName(idx) );
	txt.add( lbl );
    }

    txt.addNewLine();
    for ( int idx=0; idx<pdf->nrDims(); idx++ )
    {
	BufferString propdesc( "\n[", pdf->dimName(idx), "]\t" );
	propdesc.add( "Avg: " ).add( pdf->averagePos(idx) );
	mDynamicCastGet(const ArrayNDProbDenFunc*,sampledpdf,pdf.ptr());
	if ( sampledpdf )
	{
	    const StepInterval<float> rg = sampledpdf->getRange( idx );
	    if ( !rg.isUdf() )
	    {
		propdesc.add( ",\tMin: " ).add( rg.start );
		propdesc.add( ",\tMax: " ).add( rg.stop );
	    }
	}

	const float stdevval = pdf->stddevPos( idx );
	if ( !mIsUdf(stdevval) )
	    propdesc.add( ",\tStddev: " ).add( stdevval );

	const UnitOfMeasure* uom = UoMR().get( pdf->getUOMSymbol(idx) );
	if ( uom )
	    propdesc.add( " (" ).add( uom->getLabel() ).add( ")" );
	else
	{
	    uom = uiEditProbDenFunc::guessUnit( *pdf, idx );
	    if ( uom )
	    {
		propdesc.add( " [Guessed unit: (" )
			.add( uom->getLabel() ).add( ")]" );
	    }
	}

	txt.add( propdesc );
    }

    mDynamicCastGet(const Gaussian2DProbDenFunc*,gauss2dpdf,pdf.ptr());
    if ( gauss2dpdf )
    {
	const float corr = gauss2dpdf->getCorrelation();
	if ( !mIsUdf(corr) )
	    txt.add( "\nCorrelation: " ).add( corr );
    }

    mDynamicCastGet(const GaussianNDProbDenFunc*,gaussndpdf,pdf.ptr());
    if ( gaussndpdf )
    {
	const TypeSet<GaussianNDProbDenFunc::Corr>& corrs = gaussndpdf->corrs_;
	for ( const auto& corr : corrs )
	{
	    const float cc = corr.cc_;
	    if ( !mIsUdf(cc) )
	    {
		const BufferString varnm1 = pdf->dimName( corr.idx0_ );
		const BufferString varnm2 = pdf->dimName( corr.idx1_ );
		txt.add( "\nCorrelation " ).add( varnm1 )
		   .add( "/" ).add( varnm2 ).add( ":\t" ).add( cc );
	    }
	}
    }

    setInfo( txt );
}

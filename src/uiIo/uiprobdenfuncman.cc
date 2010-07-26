/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiprobdenfuncman.cc,v 1.8 2010-07-26 12:29:52 cvshelene Exp $";

#include "uiprobdenfuncman.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uieditpdf.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uimsg.h"

#include "bufstring.h"
#include "probdenfunc.h"
#include "probdenfunctr.h"

static const int cPrefWidth = 75;

uiProbDenFuncMan::uiProbDenFuncMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("PDF file management",
				     "Manage probability density functions",
				     "112.1.0").nrstatusflds(1),
	           ProbDenFuncTranslatorGroup::ioContext())
{
    createDefaultUI();
    selgrp->getListField()->doubleClicked.notify(
	    			mCB(this,uiProbDenFuncMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();
    manipgrp->addButton( ioPixmap("browseprdf.png"),
			 mCB(this,uiProbDenFuncMan,browsePush),
			 "Browse/edit this Probability Density Function" );

    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );
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
	    uiIOObjSelDlg dlg( this, ctio, "Save As" );
	    if ( !dlg.go() || !dlg.ioObj() ) return;
	    saveioobj = dlg.ioObj()->clone();
	}

	BufferString emsg;
	if ( !ProbDenFuncTranslator::write(*pdf,*saveioobj,&emsg) )
	    uiMSG().error( emsg );
	else
	    selgrp->fullUpdate( saveioobj->key() );
    }
}


void uiProbDenFuncMan::mkFileInfo()
{
    if ( !curioobj_ ) { infofld->setText( "" ); return; }

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

    infofld->setText( txt );
}

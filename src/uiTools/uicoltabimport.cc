/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uicoltabimport.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"

#include "iopar.h"
#include "oddirs.h"
#include "ptrman.h"
#include "filepath.h"
#include "file.h"
#include "coltabsequence.h"
#include "settings.h"


uiColTabImport::uiColTabImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import color table",0,"50.1.4"))
{
    FilePath fp( GetPersonalDir() ); fp.setFileName( 0 );
    homedirfld_ = new uiFileInput( this, "User's HOME directory",
	    			  uiFileInput::Setup(fp.fullPath())
				  .directories(true) );
    homedirfld_->valuechanged.notify( mCB(this,uiColTabImport,usrSel) );

    dtectusrfld_ = new uiGenInput( this, "DTECT_USER (if any)" );
    dtectusrfld_->attach( alignedBelow, homedirfld_ );
    dtectusrfld_->valuechanged.notify( mCB(this,uiColTabImport,usrSel) );

    listfld_ = new uiLabeledListBox( this, "Color table(s) to add", true );
    listfld_->attach( alignedBelow, dtectusrfld_ );
}


uiColTabImport::~uiColTabImport()
{
    deepErase( seqs_ );
}


const char* uiColTabImport::getCurrentSelColTab() const
{
    return listfld_->box()->getText();
}


#define mErrRet(s1) { uiMSG().error(s1); return; }

void uiColTabImport::usrSel( CallBacker* )
{
    listfld_->box()->setEmpty();

    FilePath fp( homedirfld_->fileName() );
    if ( !File::exists(fp.fullPath()) )
	mErrRet( "Please select an existing directory" );
    fp.add( ".od" );
    if ( !File::exists(fp.fullPath()) )
	mErrRet( "No '.od' directory found in directory" );

    BufferString settdir( fp.fullPath() );
    const char* dtusr = dtectusrfld_->text();

    PtrMan<IOPar> ctabiop = Settings::fetchExternal( "coltabs", dtusr, settdir);
    if ( !ctabiop )
	mErrRet( "No user-defined color tables found" );

    deepErase( seqs_ );
    for ( int idx=0; ; idx++ )
    {
	IOPar* subpar = ctabiop->subselect( idx );
	if ( !subpar || !subpar->size() )
	{
	    delete subpar;
	    if ( idx )	break;
	    else	continue;
	}
	const char* nm = subpar->find( sKey::Name() );
	if ( !nm )
	    { delete subpar; continue; }
	
	ColTab::Sequence* seq = new ColTab::Sequence;
	seq->usePar( *subpar );
	seqs_ += seq;
	ioPixmap coltabpix( *seq, 16, 10, true );
	listfld_->box()->addItem( nm, coltabpix );
    }
}


bool uiColTabImport::acceptOK( CallBacker* )
{
    bool oneadded = false;

    ObjectSet<const ColTab::Sequence> tobeadded;
    for ( int idx=0; idx<listfld_->box()->size(); idx++ )
    {
	if ( listfld_->box()->isSelected(idx) )
	    tobeadded += seqs_[idx];
    }

    for ( int idx=0; idx<tobeadded.size(); idx++ )
    {
	ColTab::Sequence seq( *tobeadded[idx] );
	bool doset = true;
	const int seqidx = ColTab::SM().indexOf( seq.name() );
	if ( seqidx >= 0 )
	{
	    BufferString msg( "User colortable '" );
	    msg += seq.name();
	    msg += "' will replace the existing.\nOverwrite?";
	    doset = uiMSG().askOverwrite( msg );
	}

	if ( doset )
	{
	    oneadded = true;
	    seq.setType( ColTab::Sequence::User );
	    ColTab::SM().set( seq );
	}
    }

    return oneadded;
}

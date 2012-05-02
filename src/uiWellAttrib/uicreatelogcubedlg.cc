/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
_______________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uicreatelogcubedlg.cc,v 1.6 2012-05-02 15:12:29 cvskris Exp $";

#include "uicreatelogcubedlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "createlogcube.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"

uiCreateLogCubeDlg::uiCreateLogCubeDlg( uiParent* p, const Well::Data& wd )
    : uiDialog(p,uiDialog::Setup("Create Log Cube",
				 "Select logs to create new cubes",
				 mTODOHelpID) )
    ,  wd_(wd)
{
    loglistfld_ = new uiListBox( this, 0, true );
    for ( int idx=0; idx<wd_.logs().size(); idx++ )
	loglistfld_->addItem( wd_.logs().getLog(idx).name() );

    repeatfld_ = new uiLabeledSpinBox( this,"Duplicate trace around the track");
    repeatfld_->attach( centeredBelow, loglistfld_);
    repeatfld_->box()->setInterval( 1, 40, 1 );

    uiSeparator* sep = new uiSeparator( this, "Save Separ", true );
    sep->attach( stretchedBelow, repeatfld_ );

    uiLabel* savelbl = new uiLabel( this, "Save CBVS cube(s)" );
    savelbl->attach( ensureBelow, sep );
    savefld_ = new uiGenInput( this, "with suffix" );
    savefld_->setElemSzPol( uiObject::Small );
    BufferString extnm( "_" ); extnm += wd_.name();
    savefld_->setText( extnm );
    savefld_->attach( rightOf, savelbl );
}


#define mErrRet( msg ) { uiMSG().error( msg ); return false; }
bool uiCreateLogCubeDlg::acceptOK( CallBacker* )
{
    const int nrsel = loglistfld_->nrSelected();
    if ( !nrsel ) 
	mErrRet( "Please select at least one log" );

    if ( !savefld_->text() )
	mErrRet( "Please enter a valid name extension for the new cubes" );

    LogCubeCreator lcr( wd_ );
    ObjectSet<LogCubeCreator::LogCubeData> logdatas;
    TypeSet<int> selidxs; loglistfld_->getSelectedItems( selidxs );
    for ( int idx=0; idx<selidxs.size(); idx++ )
    {
	const Well::Log& log = wd_.logs().getLog( selidxs[idx] );
	BufferString cbvsnm( log.name() );
	cbvsnm += savefld_->text();
	CtxtIOObj* ctio = mMkCtxtIOObj(SeisTrc);
	if ( !ctio ) continue;
	ctio->setName( cbvsnm ); 
	ctio->fillObj();
	logdatas += new LogCubeCreator::LogCubeData( log.name(), *ctio ); 
    }
    lcr.setInput( logdatas, repeatfld_->box()->getValue() );
    uiTaskRunner* tr = new uiTaskRunner( this );
    if ( !tr->execute( lcr ) || lcr.errMsg() )
	mErrRet( lcr.errMsg() );

    return true;
}


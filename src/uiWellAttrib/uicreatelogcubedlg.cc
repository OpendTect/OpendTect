/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
_______________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uicreatelogcubedlg.cc,v 1.7 2012-05-03 07:30:08 cvsbruno Exp $";

#include "uicreatelogcubedlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimultiwelllogsel.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "createlogcube.h"
#include "ioman.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"

uiCreateLogCubeDlg::uiCreateLogCubeDlg( uiParent* p, const MultiID* mid )
    : uiDialog(p,uiDialog::Setup("Create Log Cube",
				 "Select logs to create new cubes",
				 mTODOHelpID) )
{
    uiWellExtractParams::Setup su; 
    su.withzstep(false).withsampling(true).withextractintime(false);
    welllogsel_ = mid ? new uiMultiWellLogSel( this, su, *mid )
		      : new uiMultiWellLogSel( this, su );

    repeatfld_ = new uiLabeledSpinBox( this,"Duplicate trace around the track");
    repeatfld_->attach( alignedBelow, welllogsel_ );
    repeatfld_->box()->setInterval( 1, 40, 1 );

    uiSeparator* sep = new uiSeparator( this, "Save Separ", true );
    sep->attach( stretchedBelow, repeatfld_ );

    uiLabel* savelbl = new uiLabel( this, "Save CBVS cube(s)" );
    savelbl->attach( ensureBelow, sep );
    savefld_ = new uiGenInput( this, "with suffix" );
    savefld_->setElemSzPol( uiObject::Small );

    BufferString extnm( "_" );
    IOObj* ioobj = mid ? IOM().get( *mid ) : 0;
    extnm += ioobj ? ioobj->name() : "well name";

    savefld_->setText( extnm );
    savefld_->attach( rightOf, savelbl );
}


#define mErrRet( msg ) { uiMSG().error( msg ); return false; }
bool uiCreateLogCubeDlg::acceptOK( CallBacker* )
{
    if ( !savefld_->text() )
	mErrRet( "Please enter a valid name extension for the new cubes" );

    const int nrtrcs = repeatfld_->box()->getValue();
    const Well::ExtractParams& extractparams = welllogsel_->params();

    BufferStringSet wids; BufferStringSet lognms;
    welllogsel_->getSelWellIDs( wids );
    welllogsel_->getSelLogNames( lognms );
    if ( wids.isEmpty() ) 
	mErrRet( "No well selected" )

    for ( int idwell=0; idwell<wids.size(); idwell++)
    {
	Well::Data* wd = Well::MGR().get( MultiID( wids.get(idwell)) );
	if ( !wd )
	    mErrRet("Cannot read well data");

	LogCubeCreator lcr( *wd );
	ObjectSet<LogCubeCreator::LogCubeData> logdatas;
	for ( int idlog=0; idlog<lognms.size(); idlog++ )
	{
	    const char* lognm = lognms.get( idlog );
	    const Well::Log* log = wd->logs().getLog( lognm );
	    if ( !log ) continue;

	    BufferString cbvsnm( lognm );
	    cbvsnm += savefld_->text();
	    CtxtIOObj* ctio = mMkCtxtIOObj(SeisTrc);
	    if ( !ctio ) continue;
	    ctio->setName( cbvsnm ); 
	    ctio->fillObj();
	    logdatas += new LogCubeCreator::LogCubeData( lognm, *ctio ); 
	}
	lcr.setInput( logdatas, nrtrcs, extractparams ); 
	uiTaskRunner* tr = new uiTaskRunner( this );
	if ( !tr->execute( lcr ) || lcr.errMsg() )
	    mErrRet( lcr.errMsg() );
    }
    return true;
}


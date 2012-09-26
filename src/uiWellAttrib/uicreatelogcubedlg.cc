/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "iodir.h"
#include "ioman.h"
#include "seiscbvs.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"

uiCreateLogCubeDlg::uiCreateLogCubeDlg( uiParent* p, const MultiID* mid )
    : uiDialog(p,uiDialog::Setup("Create Log Cube",
				 "Select logs to create new cubes",
				 "103.2.25") )
{
    setCtrlStyle( DoAndStay );

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
    savefld_ = new uiGenInput( this, "with well name and suffix" );
    savefld_->setElemSzPol( uiObject::Small );
    savefld_->attach( rightOf, savelbl );
}


#define mErrRet( msg, act ) { uiMSG().error( msg ); act; }
bool uiCreateLogCubeDlg::acceptOK( CallBacker* )
{
    const int nrtrcs = repeatfld_->box()->getValue();
    const Well::ExtractParams& extractparams = welllogsel_->params();

    BufferStringSet wids; BufferStringSet lognms;
    welllogsel_->getSelWellIDs( wids );
    welllogsel_->getSelLogNames( lognms );
    if ( wids.isEmpty() ) 
	mErrRet("No well selected",return false);

    BufferString suffix = savefld_->text();
    for ( int idwell=0; idwell<wids.size(); idwell++)
    {
	Well::Data* wd = Well::MGR().get( MultiID( wids.get(idwell)) );
	if ( !wd )
	    mErrRet("Cannot read well data",return false);

	LogCubeCreator lcr( *wd );
	ObjectSet<LogCubeCreator::LogCubeData> logdatas;
	for ( int idlog=0; idlog<lognms.size(); idlog++ )
	{
	    const char* lognm = lognms.get( idlog );
	    const Well::Log* log = wd->logs().getLog( lognm );
	    if ( !log ) continue;

	    BufferString cbvsnm( lognm );
	    cbvsnm += " "; cbvsnm += wd->name();
	    if ( !suffix.isEmpty() )
		{ cbvsnm += " "; cbvsnm += suffix; }

	    CtxtIOObj* ctio = mMkCtxtIOObj(SeisTrc);
	    if ( !ctio ) continue;
	    ctio->ctxt.forread = false;
	    ctio->ctxt.deftransl = CBVSSeisTrcTranslator::translKey();

	    IOM().to( ctio->ctxt.getSelKey() );
	    const IOObj* presentobj = (*IOM().dirPtr())[ cbvsnm.buf() ];
	    if ( presentobj )
	    {
		BufferString msg( cbvsnm ); msg += " is already present ";

		if ( strcmp( presentobj->translator(),ctio->ctxt.deftransl) )
		{
		    msg += "as another type";
		    msg += "\n and won't be created";
		    mErrRet( msg, continue );
		}

		msg += ". \nOverwrite?";
		if ( !uiMSG().askOverwrite(msg) )
		    continue;
	    }

	    ctio->setName( cbvsnm ); 
	    ctio->fillObj();
	    logdatas += new LogCubeCreator::LogCubeData( lognm, *ctio ); 
	}
	if ( logdatas.isEmpty() )
	    return false;

	lcr.setInput( logdatas, nrtrcs, extractparams ); 
	uiTaskRunner* tr = new uiTaskRunner( this );
	if ( !tr->execute( lcr ) || lcr.errMsg() )
	    mErrRet( lcr.errMsg(), return false );
    }
    return false;
}


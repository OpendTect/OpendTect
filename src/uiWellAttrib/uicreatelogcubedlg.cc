/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
_______________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uicreatelogcubedlg.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uilistbox.h"
#include "uimultiwelllogsel.h"
#include "uispinbox.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include "createlogcube.h"
#include "iodir.h"
#include "ioman.h"
#include "seiscbvs.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"

uiCreateLogCubeDlg::uiCreateLogCubeDlg( uiParent* p, const Well::Data& wd )
    : uiDialog(p,uiDialog::Setup("Create Log Cube",
				 "Select logs to create new cubes",
				 "103.2.25") )
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
    savefld_ = new uiGenInput( this, "with well name and suffix", "_log cube" );
    savefld_->attach( rightOf, savelbl );
}


#define mErrRet( msg, act ) { uiMSG().error( msg ); act; }
bool uiCreateLogCubeDlg::acceptOK( CallBacker* )
{
    const int nrsel = loglistfld_->nrSelected();
    if ( !nrsel ) 
	mErrRet( "Please select at least one log", return false );

    LogCubeCreator lcr( wd_ );
    ObjectSet<LogCubeCreator::LogCubeData> logdatas;
    TypeSet<int> selidxs; loglistfld_->getSelectedItems( selidxs );
    BufferString suffix = savefld_->text();
    for ( int idx=0; idx<selidxs.size(); idx++ )
    {
	const Well::Log& log = wd_.logs().getLog( selidxs[idx] );
	BufferString cbvsnm( log.name() );
	cbvsnm += " "; cbvsnm += wd_.name();
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
	    {
		if ( selidxs.size() == 1 )
		    return false;
		continue;
	    }
	}

	ctio->setName( cbvsnm );
	ctio->fillObj();

	logdatas += new LogCubeCreator::LogCubeData( log.name(), *ctio );
    }
    lcr.setInput( logdatas, repeatfld_->box()->getValue()+1 );
    uiTaskRunner* tr = new uiTaskRunner( this );
    if ( !tr->execute( lcr ) || lcr.errMsg() )
	mErrRet( lcr.errMsg(), return false );

    return true;
}





uiMultiWellCreateLogCubeDlg::uiMultiWellCreateLogCubeDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create Log Cube",
		"Select logs to create new cubes",
		"103.2.22") )
{
    uiWellExtractParams::Setup su;
    su.withzstep(false).withsampling(true).withextractintime(false).withzintime(true);
    welllogsel_ = new uiMultiWellLogSel( this, su );

    repeatfld_ = new uiLabeledSpinBox( this,"Duplicate trace around the track");
    repeatfld_->attach( alignedBelow, welllogsel_ );
    repeatfld_->box()->setInterval( 1, 40, 1 );

    uiSeparator* sep = new uiSeparator( this, "Save Separ", true );
    sep->attach( stretchedBelow, repeatfld_ );

    uiLabel* savelbl = new uiLabel( this, "Save CBVS cube(s)" );
    savelbl->attach( ensureBelow, sep );
    savefld_ = new uiGenInput( this, "with well name and suffix", "_log cube" );
    savefld_->attach( rightOf, savelbl );
}


void uiMultiWellCreateLogCubeDlg::initDlg( CallBacker* )
{
    welllogsel_->update();
}


#define mMultiErrRet( msg ) { uiMSG().error( msg ); return false; }
#define mMultiErrCont( msg ) { uiMSG().error( msg ); continue; }\
{\
    if ( msg )\
	uiMSG().error( msg );\
    if ( wids.size() == 1 && lognms.size() == 1 )\
	return false;\
    continue;\
}
bool uiMultiWellCreateLogCubeDlg::acceptOK( CallBacker* )
{
    const int nrtrcs = repeatfld_->box()->getValue()+1;
    const Well::ExtractParams& extractparams = welllogsel_->params();

    BufferStringSet wids; BufferStringSet lognms;
    welllogsel_->getSelWellIDs( wids );
    welllogsel_->getSelLogNames( lognms );
    if ( wids.isEmpty() )
	mMultiErrRet( "No well selected" )

    BufferString suffix = savefld_->text();
    for ( int idwell=0; idwell<wids.size(); idwell++)
    {
	Well::Data* wd = Well::MGR().get( MultiID( wids.get(idwell)) );
	if ( !wd )
	    mMultiErrRet("Cannot read well data");

	LogCubeCreator lcr( *wd );
	ObjectSet<LogCubeCreator::LogCubeData> logdatas;
	for ( int idlog=0; idlog<lognms.size(); idlog++ )
	{
	    BufferString lognm = lognms.get( idlog );
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
		    mMultiErrCont( msg );
		}

		msg += ". \nOverwrite?";
		if ( !uiMSG().askOverwrite(msg) )
		    { mMultiErrCont(0) }
	    }

	    ctio->setName( cbvsnm );
	    ctio->fillObj();

	    logdatas += new LogCubeCreator::LogCubeData( lognms.get(idlog), 
		    					*ctio );
	}
	lcr.setInput( logdatas, nrtrcs, extractparams );
	uiTaskRunner* tr = new uiTaskRunner( this );
	if ( !tr->execute( lcr ) || lcr.errMsg() )
	    mMultiErrCont( lcr.errMsg() );
    }
    return true;
}

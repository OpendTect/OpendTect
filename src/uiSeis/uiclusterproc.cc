/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          April 2008
 RCS:		$Id: uiclusterproc.cc,v 1.1 2009-05-22 05:12:27 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiclusterproc.h"

#include "dirlist.h"
#include "timer.h"
#include "filegen.h"
#include "filepath.h"
#include "strmprov.h"
#include "envvars.h"
#include "keystrs.h"
#include "seismerge.h"
#include "settings.h"

#include "uilabel.h"
#include "uiprogressbar.h"
#include "uitaskrunner.h"
#include <iostream>


uiClusterProc::uiClusterProc( uiParent* p, const IOPar& iop )
    : uiDialog(p,uiDialog::Setup("Cluster Processing","Progress window",""))
    , pars_(iop)
    , scriptdirnm_(iop.find(uiClusterProc::sKeyScriptDir()))
{
    progbar_ = new uiProgressBar( this );
    progbar_->setPrefWidth( 500 );

    dirlist_ = new DirList( scriptdirnm_.buf(), DirList::FilesOnly, "*.scr" );
    totalnr_ = dirlist_->size();
    progbar_->setTotalSteps( totalnr_ );

    BufferString labeltxt( "No. of jobs left: " );
    labeltxt += totalnr_;
    progfld_ = new uiLabel( this, labeltxt.buf() );
    progfld_->attach( alignedBelow, progbar_ );

    timer_ = new Timer("uiClusterProc timer");
    timer_->tick.notify( mCB(this,uiClusterProc,progressCB) );
    timer_->start( 100, false );
}


uiClusterProc::~uiClusterProc()
{
std::cerr << "Deleting the window now... " << std::endl;
    delete dirlist_; delete timer_;
}


void uiClusterProc::progressCB( CallBacker* )
{
    dirlist_->update();
    const int nrleft = dirlist_->size();
    progbar_->setProgress( totalnr_ - nrleft );
    BufferString labeltxt( "No. of jobs left: " );
    labeltxt += nrleft;
    progfld_->setText( labeltxt );
    if ( totalnr_ && !nrleft )
    {
	timer_->stop();
	doPostProcessing();
    }
}


void uiClusterProc::doPostProcessing()
{
std::cerr << "Post processing now...." << std::endl;
    PtrMan<SeisMerger> exec = new SeisMerger( pars_ );
    if ( !exec )
    {
	std::cerr << "Failed to create Merger .. " << std::endl;
	return;
    }

std::cerr << "Created Merger .. " << std::endl;
    exec->stacktrcs_ = false;
    uiTaskRunner dlg( this );
    dlg.execute( *exec );
std::cerr << "Finished post-processing... " << std::endl;
}

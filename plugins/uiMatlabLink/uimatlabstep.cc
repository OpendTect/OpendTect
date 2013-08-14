/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uimatlabstep.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uitable.h"

#include "file.h"
#include "filepath.h"
#include "matlabstep.h"
#include "matlablibmgr.h"

namespace VolProc
{

uiStepDialog* uiMatlabStep::createInstance( uiParent* p, Step* step )
{
    mDynamicCastGet(MatlabStep*,ms,step);
    return ms ? new uiMatlabStep(p,ms) : 0; 
}

uiMatlabStep::uiMatlabStep( uiParent* p, MatlabStep* step )
    : uiStepDialog(p,MatlabStep::sFactoryDisplayName(), step )
    , fileloaded_(false)
{
    filefld_ = new uiFileInput( this, "Select shared object file" );
    filefld_->valuechanged.notify( mCB(this,uiMatlabStep,fileSelCB) );

    loadbut_ = new uiPushButton( this, "Load",
			mCB(this,uiMatlabStep,loadCB), true );
    loadbut_->setSensitive( false );
    loadbut_->attach( rightTo, filefld_ );

    partable_ = new uiTable( this, uiTable::Setup(5,2), "Parameter table" );
    BufferStringSet lbls; lbls.add( "Parameter" ).add( "Value" );
    partable_->setColumnLabels( lbls );
    partable_->setColumnReadOnly( 0, true );
    partable_->attach( alignedBelow, filefld_ );

    addNameFld( partable_ );

    const BufferString fnm = step->sharedLibFileName();
    if ( !fnm.isEmpty() && File::exists(fnm) )
    {
	filefld_->setFileName( fnm );
	fileSelCB( 0 );
	loadCB( 0 );
    }

    BufferStringSet parnames, parvalues;
    step->getParameters( parnames, parvalues );
    fillTable( parnames, parvalues );
}


void uiMatlabStep::fileSelCB( CallBacker* )
{
    const char* fnm = filefld_->fileName();
    const bool isok = File::exists( fnm );
    loadbut_->setSensitive( isok );
    fileloaded_ = false;

    if ( isok )
    {
	const FilePath fp( fnm );
	namefld_->setText( BufferString("MATLAB - ",fp.baseName()) );
    }
}


void uiMatlabStep::loadCB( CallBacker* )
{
    if ( fileloaded_ ) return;

    MatlabLibAccess* mla =
	MLM().getMatlabLibAccess( filefld_->fileName(), true );
    if ( !mla )
    {
	uiMSG().error( MLM().errMsg() );
	return;
    }

    BufferStringSet parnames, parvalues;
    mla->getParameters( parnames, parvalues );
    fillTable( parnames, parvalues );

    fileloaded_ = true;
}


void uiMatlabStep::fillTable( const BufferStringSet& nms,
			      const BufferStringSet& vals )
{
    partable_->clearTable();
    partable_->setNrRows( nms.size() );
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	partable_->setText( RowCol(idx,0), nms.get(idx) );
	partable_->setText( RowCol(idx,1), vals.get(idx) );
    }
}


bool uiMatlabStep::readTable( BufferStringSet& names,
			      BufferStringSet& vals ) const
{
    vals.erase();
    for ( int idx=0; idx<partable_->nrRows(); idx++ )
    {
	const FixedString nmtxt = partable_->text( RowCol(idx,0) );
	names.add( nmtxt );

	const FixedString valtxt = partable_->text( RowCol(idx,1) );
	if ( valtxt.isEmpty() )
	{
	    const char* parnm = partable_->text( RowCol(idx,0) );
	    uiMSG().error( BufferString("No value given for ",parnm) );
	    return false;
	}

	vals.add( valtxt );
    }

    return true;
}


bool uiMatlabStep::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK(cb) )
	return false;

    if ( !fileloaded_ )
    {
	loadCB( 0 );
	return false;
    }

    mDynamicCastGet(MatlabStep*,step,step_)
    if ( !step ) return false;

    step->setSharedLibFileName( filefld_->fileName() );

    BufferStringSet parnames, parvalues;
    if ( !readTable(parnames,parvalues) )
	return false;

    step->setParameters( parnames, parvalues );
    return true;
}

} // namespace VolProc

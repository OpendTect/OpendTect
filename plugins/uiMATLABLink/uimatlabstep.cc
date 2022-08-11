/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
________________________________________________________________________

-*/



#include "uimatlabstep.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistringset.h"
#include "uitable.h"

#include "envvars.h"
#include "matlabstep.h"
#include "matlablibmgr.h"

#ifdef __win__
    static const char* sofileflt = "Shared-Objects files (*.dll)";
#else
    static const char* sofileflt = "Shared-Objects files (*.so *.SO)";
#endif

namespace VolProc
{

uiStepDialog* uiMatlabStep::createInstance( uiParent* p, Step* step,
					    bool is2d )
{
    mDynamicCastGet(MatlabStep*,ms,step);
    return ms ? new uiMatlabStep(p,ms,is2d) : 0;
}

uiMatlabStep::uiMatlabStep( uiParent* p, MatlabStep* step, bool is2d )
    : uiStepDialog(p,MatlabStep::sFactoryDisplayName(), step, is2d )
    , fileloaded_(false)
{
    const FilePath sofiledir = getSODefaultDir();
    filefld_ = new uiFileInput( this, tr("Select shared object file"),
				uiFileInput::Setup(uiFileDialog::Gen)
				.filter(sofileflt)
				.defseldir(sofiledir.fullPath()) );
    filefld_->valuechanged.notify( mCB(this,uiMatlabStep,fileSelCB) );

    loadbut_ = new uiPushButton( this, uiStrings::sLoad(),
			mCB(this,uiMatlabStep,loadCB), true );
    loadbut_->setSensitive( false );
    loadbut_->attach( rightTo, filefld_ );
    uiSeparator* sep = new uiSeparator( this, "File Separator" );
    sep->attach( stretchedBelow, filefld_ );

    uiGroup* grp = new uiGroup( this, "Table Group" );
    grp->attach( alignedBelow, filefld_ );
    grp->attach( ensureBelow, sep );

    addMultiInputFld( grp );

    partable_ = new uiTable( grp, uiTable::Setup(5,2), "Parameter table" );
    uiStringSet lbls; lbls.add( tr("Parameter") ).add( tr("Value") );
    partable_->setColumnLabels( lbls );
    partable_->setColumnReadOnly( 0, true );
    partable_->attach( alignedBelow, multiinpfld_ );

    addNameFld( grp );

    if ( !step ) return;

    const BufferString fnm = step->sharedLibFileName();
    if ( fnm.isEmpty() || !File::exists(fnm) )
	return;

    filefld_->setFileName( fnm );
    fileSelCB( 0 );

    BufferStringSet parnames, parvalues;
    step->getParameters( parnames, parvalues );
    if ( !parvalues.isEmpty() )
	fillParTable( parnames, parvalues );

    setInputsFromWeb();
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
	loadCB( 0 );
    }
}


void uiMatlabStep::loadCB( CallBacker* )
{
    if ( fileloaded_ ) return;

    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    MatlabLibAccess* mla =
	MLM().getMatlabLibAccess( filefld_->fileName(), true );
    if ( !mla )
    {
	uiMSG().error( MLM().errMsg() );
	return;
    }

    int nrinputs=1, nroutputs=1;
    BufferStringSet parnames, parvalues;
    mla->getParameters( nrinputs, nroutputs, parnames, parvalues );
    fillParTable( parnames, parvalues );

    mDynamicCastGet(MatlabStep*,step,step_)
    if ( step ) step->setNrInputs( nrinputs );
    initInputTable( nrinputs );

    fileloaded_ = true;
}


void uiMatlabStep::fillParTable( const BufferStringSet& nms,
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
	const StringView nmtxt = partable_->text( RowCol(idx,0) );
	names.add( nmtxt );

	const StringView valtxt = partable_->text( RowCol(idx,1) );
	if ( valtxt.isEmpty() )
	{
	    const char* parnm = partable_->text( RowCol(idx,0) );
	    uiMSG().error( tr("No value given for %1").arg(parnm) );
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


FilePath uiMatlabStep::getSODefaultDir()
{
    StringView matlabdir( GetEnvVar( "MATLAB_BUILDDIR" ) );
    if ( matlabdir.isEmpty() )
	matlabdir = GetEnvVar( "MATLAB_DIR" );

    return FilePath( matlabdir );
}

} // namespace VolProc

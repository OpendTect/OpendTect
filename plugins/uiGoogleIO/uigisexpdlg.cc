/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : Nov 2018
-*/

#include "uigoogleexpdlg.h"
#include "uisurveymanager.h"
#include "filepath.h"
#include "uifilesel.h"
#include "oddirs.h"
#include "uimsg.h"
#include "survinfo.h"


uiGISExpStdFld::uiGISExpStdFld( uiParent* p, BufferString typnm )
    : uiGroup(p)
    , coordsysselfld_(0)
{
    StringListInpSpec writerlist( GISWriter::factory().getUserNames() );

    exptyp_ = new uiGenInput( this, sExportTypLbl(), writerlist );
    exptyp_->valuechanged.notify( mCB(this,uiGISExpStdFld,expTypChng) );

    const bool shoulddisplaycrsfld = SI().getCoordSystem() &&
	SI().getCoordSystem()->isProjection();
    if ( shoulddisplaycrsfld )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, exptyp_ );
    }

    BufferString deffnm( typnm );
    deffnm.clean( BufferString::AllowDots );
    File::Path deffp( GetDataDir() );
    deffp.add( deffnm );
    uiFileSel::Setup fssu( OD::HtmlContent, deffp.fullPath() );
    fnmfld_ = new uiFileSel( this, uiStrings::sOutputFile(), fssu );
    if ( shoulddisplaycrsfld )
	fnmfld_->attach( alignedBelow, coordsysselfld_ );
    else
	fnmfld_->attach( alignedBelow, exptyp_ );

    postFinalise().notify( mCB(this, uiGISExpStdFld, expTypChng) );
}

#define mGetWriter \
    GISWriter* wrr = GISWriter::factory().create( GISWriter::factory().key( \
				exptyp_->getIntValue()) ); \


void uiGISExpStdFld::expTypChng( CallBacker* cb )
{
    mGetWriter;

    if ( !wrr )
	return;

    File::Path fp( fnmfld_->fileName() );
    fp.setExtension( wrr->getExtension() );
    fnmfld_->setFileName( fp.fullPath() );
}


GISWriter* uiGISExpStdFld::createWriter()
{
    const BufferString fnm( fnmfld_->fileName() );
    if ( fnm.isEmpty() )
    {
	uiMSG().error( uiStrings::phrEnter(sOutFileName()) );
	return nullptr;
    }

    mGetWriter;

    if ( !wrr )
	return nullptr;
    wrr->setStream( fnm );
    if ( !wrr->isOK() )
    {
	uiMSG().error( wrr->errMsg() );
	return nullptr;
    }
    if ( coordsysselfld_ )
	wrr->setCoordSys( coordsysselfld_->getCoordSystem() );

    return wrr;
}

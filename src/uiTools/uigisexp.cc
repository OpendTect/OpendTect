/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigisexp.h"

#include "filepath.h"
#include "giswriter.h"
#include "oddirs.h"
#include "survinfo.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"


uiString uiGISExpStdFld::sToolTipTxt()
{
    return od_static_tr( "uiGISExpStdFld", "Export to GIS Format" );
}


const char* uiGISExpStdFld::strIcon()
{
    return "google";
}


OD::Color uiGISExpStdFld::sDefColor()
{
    return OD::Color::DgbColor();
}


int uiGISExpStdFld::sDefLineWidth()
{
    return 20;
}


uiGISExpStdFld::uiGISExpStdFld( uiParent* p, const char* typnm )
    : uiGroup(p)
{
    const StringListInpSpec writerlist( GIS::Writer::factory().getUserNames() );
    exptyp_ = new uiGenInput( this, tr("Export Type"), writerlist );
    mAttachCB( exptyp_->valueChanged, uiGISExpStdFld::typeChgCB );

    const bool shoulddisplaycrsfld = SI().getCoordSystem() &&
				     SI().getCoordSystem()->isProjection();
    if ( shoulddisplaycrsfld )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, exptyp_ );
    }

    BufferString deffnm( typnm );
    deffnm.clean( BufferString::AllowDots );
    FilePath deffp( GetSurveyExportDir() );
    deffp.add( deffnm );
    uiFileInput::Setup fssu( deffnm );
    fssu.forread( false ).defseldir( GetSurveyExportDir() );
    fnmfld_ = new uiFileInput( this, uiStrings::sOutputFile(), fssu );
    if ( shoulddisplaycrsfld )
	fnmfld_->attach( alignedBelow, coordsysselfld_ );
    else
	fnmfld_->attach( alignedBelow, exptyp_ );

    setHAlignObj( exptyp_ );
    mAttachCB( postFinalize(), uiGISExpStdFld::initGrpCB );
}


uiGISExpStdFld::~uiGISExpStdFld()
{
    detachAllNotifiers();
}


void uiGISExpStdFld::initGrpCB( CallBacker* )
{
    typeChgCB( nullptr );
}


void uiGISExpStdFld::typeChgCB( CallBacker* )
{
    const BufferStringSet wrrnms = GIS::Writer::factory().getNames();
    const BufferString wrrnm = wrrnms.get( exptyp_->getIntValue() );
    PtrMan<GIS::Writer> wrr = GIS::Writer::factory().create( wrrnm );
    if ( !wrr )
	return;

    FilePath fp( fnmfld_->fileName() );
    fp.setExtension( wrr->getExtension() );
    fnmfld_->setFileName( fp.fullPath() );
    fnmfld_->setSelectedFilter( wrr->getExtension() );
}


PtrMan<GIS::Writer> uiGISExpStdFld::createWriter( const char* survnm,
						  const char* elemnm ) const
{
    const BufferString fnm( fnmfld_->fileName() );
    if ( fnm.isEmpty() )
    {
	uiMSG().error( uiStrings::phrEnter(tr("Output File Name")) );
	return nullptr;
    }

    const BufferStringSet wrrnms = GIS::Writer::factory().getNames();
    const BufferString wrrnm = wrrnms.get( exptyp_->getIntValue() );
    PtrMan<GIS::Writer> wrr = GIS::Writer::factory().create( wrrnm );
    if ( !wrr )
	return nullptr;

    wrr->setSurveyName( survnm ).setElemName( elemnm ).setStream( fnm );
    if ( !wrr->isOK() )
    {
	uiMSG().error( wrr->errMsg() );
	return nullptr;
    }

    if ( coordsysselfld_ )
	wrr->setInputCoordSys( coordsysselfld_->getCoordSystem() );

    return wrr;
}


bool uiGISExpStdFld::canDoExport( uiParent* p, SurveyInfo* si )
{
    return Coords::uiCoordSystemDlg::ensureGeographicTransformOK( p, si );
}

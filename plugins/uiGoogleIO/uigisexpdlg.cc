/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleexpdlg.h"
#include "filepath.h"
#include "uifileinput.h"
#include "oddirs.h"
#include "uimsg.h"
#include "survinfo.h"


uiGISExpStdFld::uiGISExpStdFld( uiParent* p, BufferString typnm )
    : uiGroup(p)
    , coordsysselfld_(nullptr)
{
    StringListInpSpec writerlist( GISWriter::factory().getUserNames() );

    exptyp_ = new uiGenInput( this, tr("Export Type"), writerlist );
    mAttachCB(exptyp_->valueChanged,uiGISExpStdFld::expTypChng);

    const bool shoulddisplaycrsfld = SI().getCoordSystem() &&
	SI().getCoordSystem()->isProjection();
    if ( shoulddisplaycrsfld )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, exptyp_ );
    }

    BufferString deffnm( typnm );
    deffnm.clean( BufferString::AllowDots );
    FilePath deffp( GetDataDir() );
    deffp.add( deffnm );
    uiFileInput::Setup fssu( uiFileDialog::Html, deffp.fullPath() );
    fnmfld_ = new uiFileInput( this, uiStrings::sOutputFile(), fssu );
    if ( shoulddisplaycrsfld )
	fnmfld_->attach( alignedBelow, coordsysselfld_ );
    else
	fnmfld_->attach( alignedBelow, exptyp_ );

    mAttachCB(postFinalize(),uiGISExpStdFld::expTypChng);
}


uiGISExpStdFld::~uiGISExpStdFld()
{
    detachAllNotifiers();
}

void uiGISExpStdFld::expTypChng( CallBacker* )
{
    const BufferStringSet wrrnms = GISWriter::factory().getNames();
    const BufferString wrrnm = wrrnms.get( exptyp_->getIntValue() );
    PtrMan<GISWriter> wrr = GISWriter::factory().create( wrrnm );

    if ( !wrr )
	return;

    FilePath fp( fnmfld_->fileName() );
    fp.setExtension( wrr->getExtension() );
    fnmfld_->setFileName( fp.fullPath() );
}


GISWriter* uiGISExpStdFld::createWriter() const
{
    const BufferString fnm( fnmfld_->fileName() );
    if ( fnm.isEmpty() )
    {
	uiMSG().error( uiStrings::phrEnter(tr("Output File Name")) );
	return nullptr;
    }

    const BufferStringSet wrrnms = GISWriter::factory().getNames();
    const BufferString wrrnm = wrrnms.get( exptyp_->getIntValue() );
    GISWriter* wrr = GISWriter::factory().create( wrrnm );

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

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2003
-*/

#include "uiimpgprmod.h"

#include "uidialog.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uiimpexp2dgeom.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uiseissel.h"
#include "uitaskrunner.h"

#include "dztimporter.h"
#include "filepath.h"
#include "odplugin.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "posinfo2d.h"
#include "survinfo.h"
#include "survgeom2d.h"


static const char* menunm = "GPR: DZT ...";


mDefODPluginInfo(uiImpGPR)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"GPR: .DZT import",
	"OpendTect",
	"dGB (Bert Bril)",
	"=od",
	"Imports GPR data in DZT format."
	"\nThanks to Matthias Schuh (m.schuh@neckargeo.net) for information,"
	"\ntest data and comments.") );
    return &retpi;
}


class uiImpGPRMgr :  public uiPluginInitMgr
{
public:

			uiImpGPRMgr();

private:

    void		dTectMenuChanged() override;
    void		doWork(CallBacker*);
};


uiImpGPRMgr::uiImpGPRMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiImpGPRMgr::dTectMenuChanged()
{
    appl().menuMgr().getMnu( true, uiODApplMgr::Seis )->insertAction(
	new uiAction(toUiString(menunm),mCB(this,uiImpGPRMgr,doWork),"gpr") );
}


class uiDZTImporter : public uiDialog
{ mODTextTranslationClass(uiDZTImporter);
public:

uiDZTImporter( uiParent* p )
    : uiDialog(p,Setup(uiStrings::phrImport(tr("GPR-DZT Seismics")),mNoDlgTitle,
                        mODHelpKey(mDZTImporterHelpID) ))
    , inpfld_(0)
{
    setOkText( uiStrings::sImport() );

    if ( !SI().has2D() )
	{ new uiLabel(this,tr("TODO: implement 3D loading")); return; }

    uiFileSel::Setup fssu; fssu.setFormat( tr("DZT files"), "dzt" );
    inpfld_ = new uiFileSel(this, uiStrings::phrInput(tr("DZT file")), fssu );
    inpfld_->newSelection.notify( mCB(this,uiDZTImporter,inpSel) );

    nrdeffld_ = new uiGenInput( this, tr("%1 definition: start, step")
					.arg( uiStrings::sTraceNumber() ),
				IntInpSpec(1), IntInpSpec(1) );
    nrdeffld_->attach( alignedBelow, inpfld_ );
    startposfld_ = new uiGenInput( this, tr("Start position (X,Y)"),
				PositionInpSpec(SI().minCoord(OD::UsrWork)) );
    startposfld_->attach( alignedBelow, nrdeffld_ );
    stepposfld_ = new uiGenInput( this, tr("Step in X/Y"), FloatInpSpec(),
					FloatInpSpec(0) );
    stepposfld_->attach( alignedBelow, startposfld_ );

    zfacfld_ = new uiGenInput( this, tr("Z Factor"), FloatInpSpec(1) );
    zfacfld_->attach( alignedBelow, stepposfld_ );

    lnmfld_ = new uiGenInput( this, uiStrings::sLineName() );
    lnmfld_->attach( alignedBelow, zfacfld_ );

    outfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,false),
			     uiSeisSel::Setup(Seis::Line) );
    outfld_->attach( alignedBelow, lnmfld_ );
}

void inpSel( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() ) return;

    od_istream stream( fnm );

    if ( !stream.isOK() ) return;

    DZT::FileHeader fh; uiString emsg;
    if ( !fh.getFrom(stream,emsg) ) return;

    File::Path fp( fnm ); fp.setExtension( "", true );
    lnmfld_->setText( fp.fileName() );

    const float tdist = fh.spm ? 1.f / ((float)fh.spm) : SI().inlDistance();
    stepposfld_->setValue( tdist, 0 );

}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK()
{
    if ( !inpfld_ ) return true;

    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() ) mErrRet(tr("Please enter the input file name"))
    const BufferString lnm( lnmfld_->text() );
    if ( lnm.isEmpty() ) mErrRet(tr("Please enter the output line name"))

    Pos::GeomID geomid = Geom2DImpHandler::getGeomID( lnm );
    if ( mIsUdfGeomID(geomid) )
	return false;

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj ) return false;

    DZT::Importer importer( fnm, *ioobj, geomid );
    importer.fh_.nrdef_.start = nrdeffld_->getIntValue(0);
    importer.fh_.nrdef_.step = nrdeffld_->getIntValue(0);
    importer.fh_.cstart_ = startposfld_->getCoord();
    importer.fh_.cstep_.x_ = stepposfld_->getFValue(0);
    importer.fh_.cstep_.y_ = stepposfld_->getFValue(1);
    importer.zfac_ = zfacfld_->getFValue();

    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, importer );
}

    uiFileSel*		inpfld_;
    uiGenInput*		lnmfld_;
    uiGenInput*		nrdeffld_;
    uiGenInput*		startposfld_;
    uiGenInput*		stepposfld_;
    uiGenInput*		zfacfld_;
    uiSeisSel*		outfld_;

};


void uiImpGPRMgr::doWork( CallBacker* )
{
    if ( !uiSurvey::userIsOKWithPossibleTypeChange(true) ) return;

    uiDZTImporter dlg( &appl() );
    dlg.go();
}


mDefODInitPlugin(uiImpGPR)
{
    mDefineStaticLocalObject( PtrMan<uiImpGPRMgr>, theinst_,
	= new uiImpGPRMgr() );

    if ( !theinst_ )
	return "Cannot instantiate ImpGPR plugin";

    return nullptr; // All OK - no error messages
}

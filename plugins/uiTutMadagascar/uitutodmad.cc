/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          June  2009
________________________________________________________________________

-*/

#include "uitutodmad.h"

#include "file.h"
#include "keystrs.h"
#include "madio.h"
#include "madstream.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "uifilesel.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uigeninput.h"
#include "uimsg.h"

uiTutODMad::uiTutODMad( uiParent* p )
    : uiDialog( p, Setup(tr("Display Madagascar data dialog"),
			 tr("Specify data settings"), mNoHelpKey) )
    , iop_("Madagascar display parameters")
{
    uiFileSel::Setup fssu;
    fssu.initialselectiondir( ODMad::FileSpec::defPath() )
	.setFormat( tr("RSF file"), "rsf" );
    maddatafld_ = new uiFileSel( this, tr("Data file"), fssu );

    dowigglesfld_ = new uiGenInput ( this, uiStrings::sDisplay(),
			             BoolInpSpec(true,uiStrings::sWVA(),
                                     uiStrings::sVD()) );
    dowigglesfld_->attach( alignedBelow, maddatafld_ );
}


bool uiTutODMad::acceptOK()
{
    iop_.setEmpty();
    iop_.set( IOPar::compKey(sKey::Input(),sKey::Type()),
		ODMad::sKeyMadagascar() );
    const BufferString fnm = maddatafld_->fileName();
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uiMSG().error( uiStrings::phrSelect(tr("an existing input file")) );
	return false;
    }

    iop_.set( IOPar::compKey(sKey::Input(),sKey::FileName()), fnm );
    ODMad::MadStream madstream_( iop_ );
    if ( !madstream_.isOK() )
    {
	uiMSG().error( madstream_.errMsg() );
	return false;
    }

    madstream_.writeTraces( false );

    Seis::GeomType geom = madstream_.is2D()
			    ? madstream_.isPS() ? Seis::LinePS : Seis::Line
			    : madstream_.isPS() ? Seis::VolPS : Seis::Vol;
    SeisTrcBuf* trcbuf = madstream_.getTrcBuf();
    SeisTrcInfo::Fld seisifld = (SeisTrcInfo::Fld)0;
    if ( trcbuf && trcbuf->get(0) && trcbuf->get(1) )
	seisifld = trcbuf->get(0)->info().
				getDefaultAxisFld(geom,&trcbuf->get(1)->info());
    bufdtpack_ = new SeisTrcBufDataPack( trcbuf, geom, seisifld,
					 "Madagascar data" );
    bufdtpack_->trcBufArr2D().setBufMine( true );
    madstream_.setStorBufMine( false );
    DPM(DataPackMgr::FlatID()).add( bufdtpack_ );
    createAndDisplay2DViewer();
    return true;

}


void uiTutODMad::createAndDisplay2DViewer()
{
    uiFlatViewMainWin* fvwin = new uiFlatViewMainWin( 0,
		    uiFlatViewMainWin::Setup(tr("Madagascar data"),true) );
    uiFlatViewer& vwr = fvwin->viewer();
    bool dowiggles = dowigglesfld_->getBoolValue();
    vwr.setPack( dowiggles, bufdtpack_->id(), true );
    FlatView::Appearance& app = vwr.appearance();
    app.annot_.setAxesAnnot( true );
    app.setDarkBG( false );
    app.setGeoDefaults( true );
    vwr.setInitialSize( uiSize(600,400) );
    fvwin->addControl(
	    new uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(fvwin)) );
    fvwin->show();
}

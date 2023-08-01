/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutodmad.h"

#include "file.h"
#include "madio.h"
#include "madstream.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "uifileinput.h"
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
    uiFileInput::Setup fisu;
    fisu.defseldir( ODMad::FileSpec::defPath() );
    fisu.forread( true );
    maddatafld_ = new uiFileInput( this, tr("Data file"), fisu );
    maddatafld_->setFilter( "*.rsf" );

    dowigglesfld_ = new uiGenInput ( this, uiStrings::sDisplay(),
			             BoolInpSpec(true,tr("Wiggles"),
                                     tr("Variable density")) );
    dowigglesfld_->attach( alignedBelow, maddatafld_ );
}


bool uiTutODMad::acceptOK( CallBacker* )
{
    iop_.setEmpty();
    iop_.set( IOPar::compKey(sKey::Input(),sKey::Type()),
		ODMad::sKeyMadagascar() );
    const BufferString fnm = maddatafld_->fileName();
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uiMSG().error( tr("Please select an existing input file") );
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
    if ( !trcbuf || trcbuf->isEmpty() )
	return false;

    const int sz = trcbuf->size();
    SeisTrcInfo::Fld seisifld = SeisTrcInfo::SeqNr;
    if ( sz > 1 )
    {
	const SeisTrcInfo& first = trcbuf->first()->info();
	const SeisTrcInfo& next = trcbuf->get(1)->info();
	const SeisTrcInfo& last = trcbuf->last()->info();
	seisifld = first.getDefaultAxisFld( geom, &next, &last );
    }

    bufdtpack_ = new SeisTrcBufDataPack( trcbuf, geom, seisifld,
					 "Madagascar data" );
    bufdtpack_->trcBufArr2D().setBufMine( true );
    madstream_.setStorBufMine( false );
    createAndDisplay2DViewer();
    return true;

}


void uiTutODMad::createAndDisplay2DViewer()
{
    uiFlatViewMainWin* fvwin = new uiFlatViewMainWin( 0,
		    uiFlatViewMainWin::Setup(tr("Madagascar data"),true) );
    uiFlatViewer& vwr = fvwin->viewer();
    const bool dowiggles = dowigglesfld_->getBoolValue();
    const FlatView::Viewer::VwrDest dest =
	  FlatView::Viewer::getDest( dowiggles, !dowiggles );
    vwr.setPack( dest, bufdtpack_, true );
    FlatView::Appearance& app = vwr.appearance();
    app.annot_.setAxesAnnot( true );
    app.setDarkBG( false );
    app.setGeoDefaults( true );
    vwr.setInitialSize( uiSize(600,400) );
    fvwin->addControl(
	    new uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(fvwin)) );
    fvwin->show();
}

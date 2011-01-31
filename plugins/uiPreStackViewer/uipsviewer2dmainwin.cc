/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewer2dmainwin.cc,v 1.1 2011-01-31 13:03:50 cvsbruno Exp $";

#include "uipsviewer2dmainwin.h"

#include "uibutton.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uiflatviewer.h"
#include "uipsviewer2dposdlg.h"
#include "uipsviewer2d.h"

#include "prestackgather.h"
#include "seisioobjinfo.h"


namespace PreStackView 
{
    
uiViewer2DMainWin::uiViewer2DMainWin( uiParent* p )
    : uiFlatViewMainWin(p,uiFlatViewMainWin::Setup("PreStack Gather view",true))
    , posdlg_(0)
    , control_(0)	 
{
    viewer2d_ = new uiViewer2D( this );
    viewer2d_->setPrefWidth( 600 );
    viewer2d_->setPrefHeight( 600);

    uiGatherDisplay* gv = new uiGatherDisplay( 0 );
    vwrs_ += gv->getUiFlatViewer();
    viewer2d_->addGatherDisplay( gv );

    uiViewer2DControl* control = new uiViewer2DControl( this, *vwrs_[0] );
    addControl( control );
    control->posdlgcalled_.notify(mCB(this,uiViewer2DMainWin,posDlgPushed));
}


void uiViewer2DMainWin::setMultiID( const MultiID& mid )
{
    mid_ = mid;
    SeisIOObjInfo info( mid );
    info.getRanges( cs_ );
}


void uiViewer2DMainWin::posDlgPushed( CallBacker* )
{
    if ( !posdlg_ )
    {
	posdlg_ = new uiViewer2DPosDlg( this, cs_ );
	posdlg_->setCubeSampling( cs_ );
	posdlg_->show();
	posdlg_->button( uiDialog::OK )->activated.notify(
					mCB(this,uiViewer2DMainWin,setUpView) );
    }
    else
	posdlg_->show();
}


void uiViewer2DMainWin::setUpView( CallBacker* )
{
    if ( !posdlg_ ) return;
    posdlg_->getCubeSampling( cs_ );
    bool isinldir = cs_.defaultDir() == CubeSampling::Inl;
    HorSamplingIterator hsit( cs_.hrg );
    BinID bid;
    viewer2D()->removeAllGatherDisplays();
    while ( hsit.next( bid ) )
	addGather( bid );
}


void uiViewer2DMainWin::addGather( const BinID& bid )
{
    PreStack::Gather gather;
    if ( gather.readFrom( mid_, bid ) )
    {
	DPM(DataPackMgr::FlatID()).add( &gather );
	uiGatherDisplay* gv = viewer2D()->addGatherDisplay( gather.id(), bid );
	vwrs_ += gv->getUiFlatViewer();
    }
}


#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiViewer2DControl,cbnm) ); \
    tb_->addButton( but );

uiViewer2DControl::uiViewer2DControl( uiParent* p , uiFlatViewer& vwr )
    : uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(p)
			    .withthumbnail(false)
			    .withcoltabed(true)
			    .withedit(true))
    , posdlgcalled_(this)
{
    mDefBut(posbut_,"gatherdisplaysettings64.png",gatherPosCB,
	                		"Gather display positions");
}


void uiViewer2DControl::gatherPosCB( CallBacker* )
{
    posdlgcalled_.trigger();
}


void uiViewer2DControl::addGather( uiFlatViewer& gv )
{
    gathervwrs_ += &gv;
}


void uiViewer2DControl::removeGather( uiFlatViewer& gv )
{
    gathervwrs_ -= &gv;
}

}; //namepsace

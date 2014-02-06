/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurvtopbotimg.h"
#include "vistopbotimage.h"
#include "vissurvscene.h"
#include "uislider.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "vismaterial.h"

#include "survinfo.h"
#include "oddirs.h"


class uiSurvTopBotImageGrp : public uiGroup
{
public:

uiSurvTopBotImageGrp( uiSurvTopBotImageDlg* p, bool istop,
			const StepInterval<float>& zrng )
    : uiGroup(p,istop?"Top img grp":"Bot img grp")
    , dlg_(p)
    , istop_(istop)
    , img_(p->scene_->getTopBotImage(istop))
    , zrng_(zrng)
{
    uiFileInput::Setup su( uiFileDialog::Img ); su.defseldir( GetDataDir() );
    fnmfld_ = new uiFileInput( this, istop_ ? "Top image" : "Bottom image", su);
    fnmfld_->setWithCheck( true );
    fnmfld_->valuechanged.notify( mCB(this,uiSurvTopBotImageGrp,newFile) );
    mAttachCB( fnmfld_->checked,uiSurvTopBotImageGrp::onOff );
    fnmfld_->setChecked( img_->isOn() );

#define mAddCoordchgCB( notif ) \
     mAttachCB( notif, uiSurvTopBotImageGrp::coordChg );
    
    uiSliderExtra::Setup slsu( "Vertical position (Z)" );
    slsu.withedit( true );
    slsu.isvertical( true );
    slsu.sldrsize( 100 );
    zposfld_ = new uiSliderExtra( this, slsu );
    zposfld_->sldr()->setInverted( true );
    zposfld_->sldr()->setScale( zrng_.step, zrng_.start );
    zposfld_->sldr()->setInterval( zrng_ );
    zposfld_->attach( rightOf, fnmfld_ );
    zposfld_->sldr()->setValue( istop_ ? zrng_.start : zrng_.stop );
    mAddCoordchgCB( zposfld_->sldr()->valueChanged );
    
    const Coord mincrd = SI().minCoord(true);
    const Coord maxcrd = SI().maxCoord(true);
    tlfld_ = new uiGenInput( this, "NorthWest (TopLeft) Coordinate",
	    		     PositionInpSpec(Coord(mincrd.x,maxcrd.y)) );
    tlfld_->attach( leftAlignedBelow, fnmfld_ );
    mAddCoordchgCB( tlfld_->valuechanged );
    
    brfld_ = new uiGenInput( this, "SouthEast (BottomRight) Coordinate",
	    		     PositionInpSpec(Coord(maxcrd.x,mincrd.y)) );
    brfld_->attach( alignedBelow, tlfld_ );
    mAddCoordchgCB( brfld_->valuechanged );
    
    transpfld_ = new uiSliderExtra( this,
	    		    uiSliderExtra::Setup("Transparency").sldrsize(150)
			    , "Transparency slider" );
    transpfld_->attach( alignedBelow, brfld_ );
    transpfld_->sldr()->setMinValue( 0 );
    transpfld_->sldr()->setMaxValue( 100 );
    transpfld_->sldr()->setStep( 1 );
    mAttachCB( transpfld_->sldr()->valueChanged,
	       uiSurvTopBotImageGrp::transpChg );
  
    mAttachCB( postFinalise(), uiSurvTopBotImageGrp::finalisedCB ); 
}

~uiSurvTopBotImageGrp()
{
    detachAllNotifiers();
}


void finalisedCB( CallBacker* cb )
{
    fillCurrent();
}

void fillCurrent()
{
    fnmfld_->setChecked( img_->isOn() );
    const BufferString& imgnm = img_->getImageFilename();
    fnmfld_->setFileName( imgnm );
    if( img_->isOn() || !imgnm.isEmpty() )
    {
	tlfld_->setValue( img_->topLeft() );
	brfld_->setValue( img_->bottomRight() );
	transpfld_->sldr()->setValue( img_->getTransparency()*100 );
	zposfld_->sldr()->setValue( mCast(float,img_->topLeft().z) );
    }
}

void newFile( CallBacker* )
{
    dlg_->newFile( istop_, fnmfld_->fileName() );
}

void onOff( CallBacker* cb  )
{
    const bool ison = fnmfld_->isChecked();
    dlg_->setOn( istop_, ison );
    tlfld_->display( ison );
    brfld_->display( ison );
    transpfld_->display( ison );
    zposfld_->display( ison );
}

void coordChg( CallBacker* cb )
{
    const Coord3 tlcoord( tlfld_->getCoord(), zposfld_->sldr()->getValue() );
    const Coord3 brcoord( brfld_->getCoord(), zposfld_->sldr()->getValue() );
    dlg_->setCoord( istop_, tlcoord, brcoord );
}

void transpChg( CallBacker* )
{
    dlg_->setTransparency( istop_, 
			   float(transpfld_->sldr()->getIntValue()/100.) );
}

    const bool		istop_;

    uiSurvTopBotImageDlg* dlg_;
    visBase::TopBotImage* img_;

    uiFileInput*	fnmfld_;
    uiGenInput*		tlfld_;
    uiGenInput*		brfld_;
    uiSliderExtra*	transpfld_;
    uiSliderExtra*	zposfld_;
    const StepInterval<float>&	 zrng_;
};


uiSurvTopBotImageDlg::uiSurvTopBotImageDlg( uiParent* p,
					    visSurvey::Scene* scene )
    : uiDialog(p, uiDialog::Setup("Top/Bottom Images",
				  "Set Top and/or Bottom Images",
				  "50.0.13") )
    , scene_( scene )
{
    setCtrlStyle( CloseOnly );

    topfld_ = new uiSurvTopBotImageGrp( this, true,
					scene_->getCubeSampling().zrg );
    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, topfld_ );
    botfld_ = new uiSurvTopBotImageGrp( this, false,
					scene_->getCubeSampling().zrg );
    botfld_->attach( alignedBelow, topfld_ );
    botfld_->attach( ensureBelow, sep );
}



void uiSurvTopBotImageDlg::newFile( bool istop, const char* fnm )
{
    scene_->getTopBotImage(istop)->setImageFilename( fnm );
}


void uiSurvTopBotImageDlg::setOn( bool istop, bool ison )
{
    scene_->getTopBotImage(istop)->turnOn( ison );
}


void uiSurvTopBotImageDlg::setCoord( bool istop, const Coord3& tl,
						 const Coord3& br  )
{
    scene_->getTopBotImage(istop)->setPos( tl, br );
}


void uiSurvTopBotImageDlg::setTransparency( bool istop, float val )
{
    scene_->getTopBotImage(istop)->setTransparency( val );
}

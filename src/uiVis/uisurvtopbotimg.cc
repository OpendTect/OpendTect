/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisurvtopbotimg.cc,v 1.10 2012-07-30 20:57:41 cvskris Exp $";

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

uiSurvTopBotImageGrp( uiSurvTopBotImageDlg* p, bool istop )
    : uiGroup(p,istop?"Top img grp":"Bot img grp")
    , dlg_(p)
    , istop_(istop)
    , img_(p->scene_->getTopBotImage(istop))
{
    uiFileInput::Setup su( uiFileDialog::Img ); su.defseldir( GetDataDir() );
    fnmfld_ = new uiFileInput( this, istop_ ? "Top image" : "Bottom image", su);
    fnmfld_->setWithCheck( true );
    fnmfld_->valuechanged.notify( mCB(this,uiSurvTopBotImageGrp,newFile) );
    fnmfld_->checked.notify( mCB(this,uiSurvTopBotImageGrp,onOff) );

    const Coord mincrd = SI().minCoord(true);
    const Coord maxcrd = SI().maxCoord(true);
    tlfld_ = new uiGenInput( this, "NorthWest (TopLeft) Coordinate",
	    		     PositionInpSpec(Coord(mincrd.x,maxcrd.y)) );
    tlfld_->attach( alignedBelow, fnmfld_ );
    tlfld_->valuechanged.notify( mCB(this,uiSurvTopBotImageGrp,coordChg) );
    brfld_ = new uiGenInput( this, "SouthEast (BottomRight) Coordinate",
	    		     PositionInpSpec(Coord(maxcrd.x,mincrd.y)) );
    brfld_->attach( alignedBelow, tlfld_ );
    brfld_->valuechanged.notify( mCB(this,uiSurvTopBotImageGrp,coordChg) );

    transpfld_ = new uiSliderExtra( this,
	    	uiSliderExtra::Setup("Transparency"), "Transparency slider" );
    transpfld_->attach( alignedBelow, brfld_ );
    transpfld_->sldr()->setMinValue( 0 );
    transpfld_->sldr()->setMaxValue( 100 );
    transpfld_->sldr()->setStep( 1 );
    transpfld_->sldr()->valueChanged.notify(
				mCB(this,uiSurvTopBotImageGrp,transpChg) );

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
    coordChg( cb );
    transpChg( cb );
}

void coordChg( CallBacker* cb )
{
    dlg_->setCoord( istop_, tlfld_->getCoord(), brfld_->getCoord() );
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

};


uiSurvTopBotImageDlg::uiSurvTopBotImageDlg( uiParent* p,
					    visSurvey::Scene* scene )
    : uiDialog(p, uiDialog::Setup("Top/Bottom Images",
				  "Set Top and/or Bottom Images",
				  "50.0.13") )
    , scene_( scene )
{
    setCtrlStyle( LeaveOnly );

    topfld_ = new uiSurvTopBotImageGrp( this, true );
    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, topfld_ );
    botfld_ = new uiSurvTopBotImageGrp( this, false );
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


void uiSurvTopBotImageDlg::setCoord( bool istop, const Coord& tl,
				     const Coord& br  )
{
    const CubeSampling& cs = scene_->getCubeSampling();
    scene_->getTopBotImage(istop)->setPos( tl, br,
	istop ? cs.zrg.start : cs.zrg.stop );
}


void uiSurvTopBotImageDlg::setTransparency( bool istop, float val )
{
    scene_->getTopBotImage(istop)->setTransparency( val );
}

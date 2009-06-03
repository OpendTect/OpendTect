/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisurvtopbotimg.cc,v 1.2 2009-06-03 11:01:45 cvsbert Exp $";

#include "uisurvtopbotimg.h"

#include "vissurvscene.h"
#include "uislider.h"
#include "uifileinput.h"
#include "uiseparator.h"

#include "survinfo.h"
#include "oddirs.h"


class uiSurvTopBotImageGrp : public uiGroup
{
public:

uiSurvTopBotImageGrp( uiSurvTopBotImageDlg* p, bool istop )
    : uiGroup(p,istop?"Top img grp":"Bot img grp")
    , dlg_(p)
    , istop_(istop)
    , scene_(p->scene_)
{
    uiFileInput::Setup su( uiFileDialog::Img ); su.defseldir( GetDataDir() );
    fnmfld_ = new uiFileInput( this, istop_ ? "Top image" : "Bottom image", su);
    fnmfld_->setWithCheck( true );
    fnmfld_->valuechanged.notify( mCB(this,uiSurvTopBotImageGrp,newFile) );
    fnmfld_->checked.notify( mCB(this,uiSurvTopBotImageGrp,onOff) );

    tlfld_ = new uiGenInput( this, "NorthWest (TopLeft) Coordinate",
	    		     PositionInpSpec(SI().minCoord(false)) );
    tlfld_->attach( alignedBelow, fnmfld_ );
    tlfld_->valuechanged.notify( mCB(this,uiSurvTopBotImageGrp,coordChg) );
    brfld_ = new uiGenInput( this, "SouthEast (BottomRight) Coordinate",
	    		     PositionInpSpec(SI().maxCoord(false)) );
    brfld_->attach( alignedBelow, tlfld_ );
    brfld_->valuechanged.notify( mCB(this,uiSurvTopBotImageGrp,coordChg) );

    transpfld_ = new uiSliderExtra( this,
	    	uiSliderExtra::Setup("Transparency"), "Transparency slider" );
    transpfld_->attach( alignedBelow, brfld_ );
    transpfld_->sldr()->setMinValue( 0 );
    transpfld_->sldr()->setMaxValue( 255 );
    transpfld_->sldr()->setStep( 1 );
    transpfld_->sldr()->valueChanged.notify(
				mCB(this,uiSurvTopBotImageGrp,transpChg) );

    fillCurrent();
}

void fillCurrent()
{
    //TODO set file names, coords etc. as in current display, like:
    // fnmfld_->setChecked( have_image );
    // transpfld_->sldr()->setValue( initaltransp );
}

void newFile( CallBacker* )
{
    dlg_->newFile( istop_, fnmfld_->fileName() );
}

void onOff( CallBacker* )
{
    const bool ison = fnmfld_->isChecked();
    dlg_->setOn( istop_, ison );
    tlfld_->display( ison );
    brfld_->display( ison );
    transpfld_->display( ison );
}

void coordChg( CallBacker* cb )
{
    const bool istl = cb == tlfld_;
    dlg_->setCoord( istop_, istl,
	    	    istl ? tlfld_->getCoord() : brfld_->getCoord() );
}

void transpChg( CallBacker* )
{
    dlg_->setTransp( istop_, transpfld_->sldr()->getIntValue() );
}

    const bool		istop_;

    uiSurvTopBotImageDlg* dlg_;
    visSurvey::Scene*	scene_;

    uiFileInput*	fnmfld_;
    uiGenInput*		tlfld_;
    uiGenInput*		brfld_;
    uiSliderExtra*	transpfld_;

};


uiSurvTopBotImageDlg::uiSurvTopBotImageDlg( uiParent* p,
					    visSurvey::Scene* scene )
    : uiDialog(p, uiDialog::Setup("Top/Bottom Images",
				  "Set Top and/or Bottom Images",
				  mTODOHelpID) )
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
//TODO user has selected a new file name
}


void uiSurvTopBotImageDlg::setOn( bool istop, bool ison )
{
//TODO user switches on or off
}


void uiSurvTopBotImageDlg::setCoord( bool istop, bool istopleft,
				     const Coord& pos )
{
//TODO user changes a corner coordinate
}


void uiSurvTopBotImageDlg::setTransp( bool istop, int val )
{
//TODO user changes the transparency slider
}

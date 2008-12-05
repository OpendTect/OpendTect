/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldispprop.cc,v 1.2 2008-12-05 15:20:05 cvsbert Exp $";

#include "uiwelldispprop.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uispinbox.h"
#include "uigeninput.h"
#include "uilabel.h"


uiWellDispProperties::uiWellDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::BasicProps& pr )
    : uiGroup(p,"Well display properties group")
    , props_(pr)
{
    szfld_ = new uiSpinBox( this, 0, "Size" );
    szfld_->setInterval( StepInterval<int>(1,mUdf(int),1) );
    new uiLabel( this, su.sztxt_ , szfld_ );

    uiColorInput::Setup csu( props_.color_ );
    csu.lbltxt( su.coltxt_ ).withalpha( false );
    BufferString dlgtxt( "Select " );
    dlgtxt += su.coltxt_; dlgtxt += " for "; dlgtxt += props_.subjectName();
    colfld_ = new uiColorInput( this, csu, su.coltxt_ );
    colfld_->attach( alignedBelow, szfld_ );

    setHAlignObj( colfld_ );
}


void uiWellDispProperties::putToScreen()
{
    szfld_->setValue( props_.size_ );
    colfld_->setColor( props_.color_ );
    doPutToScreen();
}


void uiWellDispProperties::getFromScreen()
{
    props_.size_ = szfld_->getValue();
    props_.color_ = colfld_->color();
    doGetFromScreen();
}


uiWellTrackDispProperties::uiWellTrackDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Track& tp )
    : uiWellDispProperties(p,su,tp)
{
    dispabovefld_ = new uiCheckBox( this, "Above" );
    dispabovefld_->attach( alignedBelow, colfld_ );
    dispbelowfld_ = new uiCheckBox( this, "Below" );
    dispbelowfld_->attach( rightOf, dispabovefld_ );
    uiLabel* lbl = new uiLabel( this, "Display well name" , dispabovefld_ );
    lbl = new uiLabel( this, "track" );
    lbl->attach( rightOf, dispbelowfld_ );
}


void uiWellTrackDispProperties::doPutToScreen()
{
    dispabovefld_->setChecked( trackprops().dispabove_ );
    dispbelowfld_->setChecked( trackprops().dispbelow_ );
}


void uiWellTrackDispProperties::doGetFromScreen()
{
    trackprops().dispabove_ = dispabovefld_->isChecked();
    trackprops().dispbelow_ = dispbelowfld_->isChecked();
}


uiWellMarkersDispProperties::uiWellMarkersDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Markers& mp )
    : uiWellDispProperties(p,su,mp)
{
    circfld_ = new uiGenInput( this, "Shape",
			       BoolInpSpec(true,"Circular","Square") );
    circfld_->attach( alignedBelow, colfld_ );
}


void uiWellMarkersDispProperties::doPutToScreen()
{
    circfld_->setValue( mrkprops().circular_ );
}


void uiWellMarkersDispProperties::doGetFromScreen()
{
    mrkprops().circular_ = circfld_->getBoolValue();
}

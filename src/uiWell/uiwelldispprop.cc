/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldispprop.cc,v 1.4 2008-12-10 10:03:21 cvsbruno Exp $";

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
    , propChanged(this)
{
    szfld_ = new uiSpinBox( this, 0, "Size" );
    szfld_->setInterval( StepInterval<int>(1,mUdf(int),1) );
    szfld_->valueChanging.notify( mCB(this,uiWellDispProperties,propChg) );
    new uiLabel( this, su.sztxt_ , szfld_ );

    uiColorInput::Setup csu( props_.color_ );
    csu.lbltxt( su.coltxt_ ).withalpha( false );
    BufferString dlgtxt( "Select " );
    dlgtxt += su.coltxt_; dlgtxt += " for "; dlgtxt += props_.subjectName();
    colfld_ = new uiColorInput( this, csu, su.coltxt_ );
    colfld_->attach( alignedBelow, szfld_ );
    colfld_->colorchanged.notify( mCB(this,uiWellDispProperties,propChg) );

    setHAlignObj( colfld_ );
}


void uiWellDispProperties::propChg( CallBacker* )
{
    getFromScreen();
    propChanged.trigger();
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
    dispabovefld_->activated.notify( mCB(this,uiWellTrackDispProperties,propChg) );
    dispbelowfld_ = new uiCheckBox( this, "Below" );
    dispbelowfld_->attach( rightOf, dispabovefld_ );
    dispbelowfld_->activated.notify( mCB(this,uiWellTrackDispProperties,propChg) );
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
    circfld_->valuechanged.notify( mCB(this,uiWellMarkersDispProperties,propChg) );
}


void uiWellMarkersDispProperties::doPutToScreen()
{
    circfld_->setValue( mrkprops().circular_ );
}


void uiWellMarkersDispProperties::doGetFromScreen()
{
    mrkprops().circular_ = circfld_->getBoolValue();
}


uiWellLogDispProperties::uiWellLogDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Log& lp )
    : uiWellDispProperties(p,su,lp)
{
    rangefld_ = new uiGenInput( this, "data range",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    rangefld_->attach( alignedAbove, szfld_ );
   
    const char* choice[] = { "cliprate", "data range", 0 };
    cliprangefld_ = new uiGenInput( this, "Specify",
				StringListInpSpec(choice) );
    cliprangefld_->attach( alignedAbove, rangefld_ );
   
    stylefld_ = new uiGenInput( this, "Style", 
			       BoolInpSpec( true,"Well log","Seismic" ) );
    stylefld_->attach( alignedAbove, cliprangefld_ );


/*
    clipratefld_ = new uiGenInput( this, "Clip rate", StringInpSpec() );
    clipratefld_->setElemSzPol( uiObject::Small );
    clipratefld_->attach( alignedAbove, szfld_ );
    clipratefld_->display(false);*/
}


void uiWellLogDispProperties::doPutToScreen()
{
    stylefld_->setValue( logprops().seismicstyle_ ); 
    clipratefld_->setValue( logprops().seismicstyle_ ); 
    rangefld_->setValue( logprops().seismicstyle_ ); 
    cliprangefld_->setValue( logprops().seismicstyle_ ); 
}


void uiWellLogDispProperties::doGetFromScreen()
{
    logprops().seismicstyle_ = stylefld_->getBoolValue();
    logprops().range_ = rangefld_->getFInterval();
   // logprops().logarithmic_ = logscfld_->isChecked();
  //  logprops().linecolor_ = colorfld_->color();
   // logprops().seisstyle_ = logstylefld_->getBoolValue();
  //  logprops().repeat_ = repeatfld_->isChecked();
  //  logprops().repeat_ = repeatfld_->getIntValue();
  //  logprops().repeatovlap_ = ovlapfld_->getfValue();
   // logprops().logfillcolor_ =  colorseisfld_->color();
   // logprops().logfill_ = logfillfld_->isChecked();
    logprops().cliprate_ = clipratefld_->getfValue();
    logprops().iscliprate_ = cliprangefld_->getIntValue();
  //  logprops().seqname_ = coltablistfld_-> text();
//    logprops().singlfillcol_ = singlfillcolfld_->isChecked();

}


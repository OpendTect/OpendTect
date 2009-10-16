/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldispprop.cc,v 1.31 2009-10-16 09:15:14 cvsnanne Exp $";

#include "uiwelldispprop.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"

#include "coltabsequence.h"
#include "multiid.h"
#include "pixmap.h"
#include "welldata.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"

static int deflogwidth = 30;


uiWellDispProperties::uiWellDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::BasicProps& pr )
    : uiGroup(p,"Well display properties group")
    , props_(pr)
    , propChanged(this)

{
    szfld_ = new uiSpinBox( this, 0, "Size" );
    szfld_->setInterval( StepInterval<int>(0,mUdf(int),1) );
    szfld_->setValue(  props_.size_ );
    szfld_->valueChanging.notify( mCB(this,uiWellDispProperties,propChg) );
    new uiLabel( this, su.mysztxt_, szfld_ );

    uiColorInput::Setup csu( props_.color_ );
    csu.lbltxt( su.mycoltxt_ ).withalpha( false );
    BufferString dlgtxt( "Select " );
    dlgtxt += su.mycoltxt_; dlgtxt += " for "; dlgtxt += props_.subjectName();
    colfld_ = new uiColorInput( this, csu, su.mycoltxt_ );
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
    dispabovefld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    dispbelowfld_ = new uiCheckBox( this, "Below" );
    dispbelowfld_->attach( rightOf, dispabovefld_ );
    dispbelowfld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    uiLabel* lbl = new uiLabel( this, "Display well name" , dispabovefld_ );
    lbl = new uiLabel( this, "track" );
    lbl->attach( rightOf, dispbelowfld_ );

    nmsizefld_ = new uiLabeledSpinBox( this, "Name size" );
    nmsizefld_->box()->setInterval(10,30,6);
    nmsizefld_->box()->valueChanging.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    nmsizefld_->attach( alignedBelow, dispabovefld_  );
    doPutToScreen();
}


void uiWellTrackDispProperties::doPutToScreen()
{
    NotifyStopper nsa( dispabovefld_->activated);
    NotifyStopper nsb( dispbelowfld_->activated);
    dispbelowfld_->setChecked( trackprops().dispbelow_ );
    dispabovefld_->setChecked( trackprops().dispabove_ );
    nmsizefld_->box()->setValue( trackprops().nmsize_ );
}


void uiWellTrackDispProperties::doGetFromScreen()
{
    trackprops().dispbelow_ = dispbelowfld_->isChecked();
    trackprops().dispabove_ = dispabovefld_->isChecked();
    trackprops().nmsize_ =  nmsizefld_->box()->getValue();
}



uiWellMarkersDispProperties::uiWellMarkersDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Markers& mp )
    : uiWellDispProperties(p,su,mp)
{
    circfld_ = new uiGenInput( this, "Shape",
			       BoolInpSpec(true,"Square","Circular") );
    circfld_->attach( alignedBelow, colfld_ );
    circfld_->valuechanged.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
   
    singlecolfld_ = new uiCheckBox( this, "use single color");
    singlecolfld_ -> attach( rightOf, colfld_); 
    colfld_->setSensitive( singlecolfld_->isChecked() );
    singlecolfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    singlecolfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,setMarkerColSel));
   
    nmsizefld_ = new uiLabeledSpinBox( this, "Names size" );
    nmsizefld_->box()->setInterval(10,30,6);
    nmsizefld_->box()->valueChanging.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    nmsizefld_->attach( alignedBelow, circfld_ );
    
    uiColorInput::Setup csu( mrkprops().color_ );
    BufferString dlgtxt( "Names color" );
    csu.lbltxt( dlgtxt ).withalpha( false );
    nmcolfld_ = new uiColorInput( this, csu, dlgtxt );
    nmcolfld_->attach( alignedBelow, nmsizefld_ );
    nmcolfld_->colorchanged.notify( 
		mCB(this,uiWellMarkersDispProperties,propChg) );

    samecolasmarkerfld_ = new uiCheckBox( this, "same as markers");
    samecolasmarkerfld_->attach( rightOf, nmcolfld_); 
    samecolasmarkerfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    samecolasmarkerfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,setMarkerNmColSel));
   
    doPutToScreen();
}


void uiWellMarkersDispProperties::setMarkerColSel( CallBacker*  )
{
    bool issel = singlecolfld_->isChecked();
    colfld_->setSensitive( issel );
}


void uiWellMarkersDispProperties::setMarkerNmColSel( CallBacker*  )
{
    bool issel = samecolasmarkerfld_->isChecked();
    nmcolfld_->setSensitive( !issel );
}

void uiWellMarkersDispProperties::doPutToScreen()
{
    circfld_->setValue( mrkprops().circular_ );
    singlecolfld_->setChecked( mrkprops().issinglecol_ );
    nmsizefld_->box()->setValue( mrkprops().nmsize_ );
    samecolasmarkerfld_->setChecked( mrkprops().samenmcol_ );
    nmcolfld_->setColor( mrkprops().nmcol_ );
}


void uiWellMarkersDispProperties::doGetFromScreen()
{
    mrkprops().circular_ = circfld_->getBoolValue();
    mrkprops().issinglecol_ = singlecolfld_->isChecked();
    mrkprops().nmsize_ =  nmsizefld_->box()->getValue();
    mrkprops().samenmcol_ = samecolasmarkerfld_->isChecked();
    mrkprops().nmcol_ =  nmcolfld_->color();
}


uiWellLogDispProperties::uiWellLogDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Log& lp, 
				Well::LogSet* wl)
    : uiWellDispProperties(p,su,lp)
    , wl_(wl)
{
    CallBack propchgcb = mCB(this,uiWellLogDispProperties,propChg);
    CallBack choiceselcb = mCB(this,uiWellLogDispProperties,choiceSel);
    stylefld_ = new uiGenInput( this, "Style", 
			        BoolInpSpec(true,"Well log","Seismic") );
    stylefld_->attach( alignedAbove, szfld_ );
    stylefld_->valuechanged.notify(
		mCB(this,uiWellLogDispProperties,isSeismicSel) );
    stylefld_->valuechanged.notify(
		mCB(this,uiWellLogDispProperties,isStyleChanged) );
    stylefld_->valuechanged.notify( propchgcb );

    rangefld_ = new uiGenInput( this, "Data range",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    rangefld_->attach( alignedAbove, stylefld_ );
    rangefld_->valuechanged.notify( choiceselcb );
    rangefld_->valuechanged.notify( propchgcb );
   
    const char* choice[] = { "clip rate", "data range", 0 };
    cliprangefld_ = new uiGenInput( this, "Specify", StringListInpSpec(choice));
    cliprangefld_->attach( alignedAbove, rangefld_ );
    cliprangefld_->valuechanged.notify( choiceselcb );
   
    clipratefld_ = new uiGenInput( this, "Clip rate", StringInpSpec() );
    clipratefld_->setElemSzPol( uiObject::Small );
    clipratefld_->attach( alignedBelow, cliprangefld_ );
    clipratefld_->valuechanged.notify( choiceselcb );
    clipratefld_->valuechanged.notify( propchgcb );

    logarithmfld_ = new uiCheckBox( this, "Logarithmic" );
    logarithmfld_->setName( BufferString("Logarithmic") );
    logarithmfld_->attach( rightOf, rangefld_ );
    logarithmfld_->activated.notify( propchgcb );

    logfillfld_ = new uiCheckBox( this, "log filled" );
    logfillfld_->attach( rightOf, colfld_ );   
    logfillfld_->activated.notify(
		mCB(this,uiWellLogDispProperties,isFilledSel));
    logfillfld_->activated.notify( propchgcb );

    singlfillcolfld_ = new uiCheckBox( this, "single color" );
    singlfillcolfld_->setName( BufferString("single color") );
    singlfillcolfld_->attach(rightOf, logfillfld_);
    singlfillcolfld_->activated.notify(
		mCB(this,uiWellLogDispProperties,isFilledSel) );
    singlfillcolfld_->activated.notify( propchgcb );

    BufferStringSet lognames;
    for ( int idx=0; idx< wl_->size(); idx++ )
	lognames.addIfNew( wl_->getLog(idx).name() );
    lognames.sort();
    BufferString sellbl( "Select log" );
    logsfld_ = new uiLabeledComboBox( this, sellbl );
    logsfld_->box()->setHSzPol( uiObject::Wide );
    logsfld_->box()->addItem("None");
    logsfld_->box()->addItems( lognames );
    logsfld_->attach( alignedAbove, cliprangefld_ );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,logSel) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateRange) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );

    BufferString selfilllbl( "Fill with log" );
    filllogsfld_ = new uiLabeledComboBox( this, selfilllbl );
    filllogsfld_->box()->addItems( lognames );
    filllogsfld_->attach( alignedBelow, colfld_ );
    filllogsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );

    coltablistfld_ = new uiColorTableSel( this, "Table selection" );
    coltablistfld_->attach( rightOf, filllogsfld_ );
    coltablistfld_->selectionChanged.notify( propchgcb );

    colorrangefld_ = new uiGenInput( this, "Color range",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    colorrangefld_->attach( alignedBelow, filllogsfld_ );
    colorrangefld_->valuechanged.notify( choiceselcb );
    colorrangefld_->valuechanged.notify( propchgcb );

    logwidthfld_ = new uiLabeledSpinBox( this, "Log screen width" );
    logwidthfld_->box()->setInterval(1,500);
    logwidthfld_->box()->valueChanging.notify( propchgcb );
    logwidthfld_->attach( rightOf, szfld_ );

    seiscolorfld_ = new uiColorInput( this,
		                 uiColorInput::Setup(logprops().seiscolor_)
			        .lbltxt("Filling color") );
    seiscolorfld_->attach( alignedBelow, logwidthfld_);
    seiscolorfld_->display(false);
    seiscolorfld_->colorchanged.notify( propchgcb );

    fillcolorfld_ = new uiColorInput( this,
		                 uiColorInput::Setup(logprops().seiscolor_)
			        .lbltxt("Filling color") );
    fillcolorfld_->attach( alignedBelow, colfld_);
    fillcolorfld_->display(false);
    fillcolorfld_->colorchanged.notify( propchgcb );

    lblr_ = new uiLabeledSpinBox( this, "Repeat" );
    repeatfld_ = lblr_ ->box();
    repeatfld_->setInterval( 1, 20, 1 );
    repeatfld_->valueChanging.notify(
		mCB(this,uiWellLogDispProperties,isRepeatSel) );
    lblr_->attach(alignedBelow, colfld_);
    repeatfld_->valueChanging.notify( propchgcb );
  
    lblo_ = new uiLabeledSpinBox( this, "Overlap" );
    ovlapfld_ = lblo_ ->box();
    ovlapfld_->setInterval( 0, 100, 20 );
    lblo_->attach( alignedBelow, seiscolorfld_ );
    ovlapfld_->valueChanging.notify( propchgcb );
    
    recoverProp();
}


void uiWellLogDispProperties::doPutToScreen()
{
    NotifyStopper nssfc( singlfillcolfld_->activated );
    NotifyStopper nsslf( logfillfld_->activated );
    NotifyStopper nso( ovlapfld_->valueChanging );
    NotifyStopper nsr( repeatfld_->valueChanging );
    NotifyStopper nsl( logarithmfld_->activated );

    logsfld_->box()-> setText( logprops().name_ );
    rangefld_->setValue( logprops().range_ );
    colorrangefld_->setValue( logprops().fillrange_ );
    filllogsfld_->box()-> setText( logprops().fillname_ );
    stylefld_->setValue( logprops().iswelllog_ );
    logarithmfld_->setChecked( logprops().islogarithmic_ ); 
    coltablistfld_->setText( logprops().seqname_ ); 
    ovlapfld_->setValue( logprops().repeatovlap_ );
    repeatfld_->setValue( logprops().repeat_ );
    logfillfld_->setChecked( logprops().islogfill_ );
    singlfillcolfld_->setChecked( logprops().issinglecol_ ); 
    clipratefld_->setValue( logprops().cliprate_ );
    cliprangefld_->setValue( logprops().isdatarange_ );
    if ( mIsUdf( logprops().cliprate_) || logprops().cliprate_ > 100  )
    {
	cliprangefld_->setValue( true );
	clipratefld_->setValue( 0.0 );
    }
    logwidthfld_->box()->setValue( logprops().logwidth_ );
    if (logprops().iswelllog_ )
	fillcolorfld_->setColor( logprops().seiscolor_ );
    else
	seiscolorfld_->setColor( logprops().seiscolor_ );
}


void uiWellLogDispProperties::doGetFromScreen()
{
    logprops().iswelllog_ = stylefld_->getBoolValue();
    logprops().isdatarange_ = cliprangefld_->getBoolValue();
    logprops().cliprate_ = clipratefld_->getfValue();
    if ( mIsUdf( logprops().cliprate_) || logprops().cliprate_ > 100 )
    {
	logprops().cliprate_= 0.0;
        logprops().isdatarange_ = true;
    }
    logprops().range_ = rangefld_->getFInterval();
    logprops().fillrange_ = colorrangefld_->getFInterval();
    logprops().islogarithmic_ = logarithmfld_->isChecked(); 
    logprops().islogfill_ = logfillfld_->isChecked();
    logprops().issinglecol_ = singlfillcolfld_->isChecked();
    logprops().seqname_ = coltablistfld_-> text();
    if ( stylefld_->getBoolValue() == true )
	logprops().repeat_ = 1;
    else
	logprops().repeat_ = repeatfld_->getValue();
    logprops().repeatovlap_ = ovlapfld_->getValue();
    if (logprops().iswelllog_ )
	logprops().seiscolor_ = fillcolorfld_->color();
    else
	logprops().seiscolor_ = seiscolorfld_->color();
    logprops().name_ = logsfld_->box()->text();
    logprops().fillname_ = filllogsfld_->box()->text();
    deflogwidth = logprops().logwidth_ = logwidthfld_->box()->getValue();
}


void uiWellLogDispProperties::isFilledSel( CallBacker* )
{
    const bool iswelllog = stylefld_->getBoolValue();
    const bool isfilled = logfillfld_->isChecked();
    const bool issinglecol = singlfillcolfld_->isChecked();
    singlfillcolfld_->display( isfilled && iswelllog);
    coltablistfld_->display( iswelllog &&  isfilled && !issinglecol );
    seiscolorfld_->display( !iswelllog );
    fillcolorfld_->display( (issinglecol && isfilled) );
    filllogsfld_->display( iswelllog &&  isfilled && !issinglecol );
    colorrangefld_->display( iswelllog &&  isfilled && !issinglecol );
}


void uiWellLogDispProperties::isRepeatSel( CallBacker* )
{
    const bool isrepeat =  repeatfld_->getValue();
    const bool iswelllog = stylefld_->getBoolValue();
    if ( iswelllog )
	repeatfld_-> setValue( 1 );
}


void uiWellLogDispProperties::isSeismicSel( CallBacker* )
{
    const bool iswelllog = stylefld_->getBoolValue();
    lblr_->display( !iswelllog );
    lblo_->display( !iswelllog );
    logfillfld_->display( iswelllog );
    if (iswelllog)
	repeatfld_->setValue(1);
    BufferString str = "Select ";
    str += "filling color";
    isFilledSel(0);
}


void uiWellLogDispProperties::isStyleChanged( CallBacker* )
{
    seiscolorfld_->setColor( Color::White() );
    fillcolorfld_->setColor( Color::White() );
    const bool iswelllog = stylefld_->getBoolValue();
    const bool singlecol = singlfillcolfld_->isChecked();
    fillcolorfld_ -> display( iswelllog && singlecol );
    seiscolorfld_ -> display( !iswelllog );
}


void uiWellLogDispProperties::recoverProp( )
{
    doPutToScreen();
    if ( logprops().name_ == "None" || logprops().name_ ==  "none" ) selNone();
    isSeismicSel(0);
    choiceSel(0);
    isFilledSel(0);
}


void uiWellLogDispProperties::setRangeFields( Interval<float>& range )
{
}


void uiWellLogDispProperties::logSel( CallBacker* )
{
    setFieldVals( false );
    BufferString fillname = filllogsfld_->box()->text();
    filllogsfld_-> box() -> setText( logsfld_->box()->text() );
}


void uiWellLogDispProperties::selNone()
{
    rangefld_->setValue( Interval<float>(0,0) );
    colorrangefld_->setValue( Interval<float>(0,0) );
    colfld_->setColor( Color::White() );
    seiscolorfld_->setColor( Color::White() );
    fillcolorfld_->setColor( Color::White() );
    stylefld_->setValue( true );
    setFldSensitive( false );
    cliprangefld_->setValue( true );
    clipratefld_->setValue( 0.0 );
    repeatfld_->setValue( 0 );
    ovlapfld_->setValue( 0 );
    logfillfld_->setChecked( true );
    singlfillcolfld_->setChecked( false );
    coltablistfld_->setText( "Rainbow" ); 
    logwidthfld_->box()->setValue( deflogwidth );
}



void uiWellLogDispProperties::setFldSensitive( bool yn )
{
    rangefld_->setSensitive( yn );
    colorrangefld_->setSensitive( yn );
    cliprangefld_->setSensitive( yn );
    colfld_->setSensitive( yn );
    seiscolorfld_->setSensitive( yn );
    fillcolorfld_->setSensitive( yn );
    stylefld_->setSensitive( yn );
    clipratefld_->setSensitive( yn );
    lblr_->setSensitive( yn );
    logfillfld_->setSensitive( yn );
    szfld_->setSensitive( yn );
    singlfillcolfld_->setSensitive( yn );
    coltablistfld_->setSensitive( yn );
    filllogsfld_->setSensitive(yn);
    logarithmfld_->setSensitive(yn);
    logwidthfld_->setSensitive(yn);
}


void uiWellLogDispProperties::choiceSel( CallBacker* )
{
    const int isdatarange = cliprangefld_->getBoolValue();
    rangefld_->display( isdatarange );
    clipratefld_->display( !isdatarange );
}


void uiWellLogDispProperties::setFieldVals( bool def )
{
    BufferString sel = logsfld_->box()->text();
    if ( sel == "None")
    {
	selNone();
	return;
    }
    setFldSensitive( true );
}


void uiWellLogDispProperties::updateRange( CallBacker* )
{
    const char* lognm = logsfld_->box()->textOfItem(
		        logsfld_->box()->currentItem() ); 
    const int logno = wl_->indexOf( lognm );
    if ( logno<0 ) return; 

    rangefld_->setValue( wl_->getLog(logno).valueRange() );
    propChanged.trigger();
}


void uiWellLogDispProperties::updateFillRange( CallBacker* )
{
    const char* lognm = filllogsfld_->box()->textOfItem(
			filllogsfld_->box()->currentItem() ); 
    const int logno = wl_->indexOf( lognm );
    if ( logno<0 ) return; 

    colorrangefld_->setValue( wl_->getLog(logno).valueRange() );
    propChanged.trigger();
}



void uiWellLogDispProperties::calcRange( const char* lognm,
					 Interval<float>& valr )
{
    valr.set( mUdf(float), -mUdf(float) );
    for ( int idy=0; idy<wl_->size(); idy++ )
    {
	if ( !strcmp(lognm, wl_->getLog(idy).name()) )
	{
	    const int logno = wl_->indexOf( lognm );
	    Interval<float> range = wl_->getLog(logno).valueRange();
	    if ( valr.start > range.start )
		valr.start = range.start;
	    if ( valr.stop < range.stop )
		valr.stop = range.stop;
	}
    }
}


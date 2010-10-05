/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldispprop.cc,v 1.43 2010-10-05 12:37:34 cvsbruno Exp $";

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
    , props_(&pr)
    , propChanged(this)

{
    szfld_ = new uiSpinBox( this, 0, "Size" );
    szfld_->setInterval( StepInterval<int>(0,100,1) );
    szfld_->setValue(  props().size_ );
    szfld_->valueChanging.notify( mCB(this,uiWellDispProperties,propChg) );
    new uiLabel( this, su.mysztxt_, szfld_ );

    uiColorInput::Setup csu( props().color_ );
    csu.lbltxt( su.mycoltxt_ ).withalpha( false );
    BufferString dlgtxt( "Select " );
    dlgtxt += su.mycoltxt_; dlgtxt += " for "; dlgtxt += props().subjectName();
    colfld_ = new uiColorInput( this, csu, su.mycoltxt_ );
    colfld_->attach( alignedBelow, szfld_ );
    colfld_->colorChanged.notify( mCB(this,uiWellDispProperties,propChg) );

    setHAlignObj( colfld_ );
}


void uiWellDispProperties::propChg( CallBacker* )
{
    getFromScreen();
    propChanged.trigger();
}


void uiWellDispProperties::putToScreen()
{
    szfld_->setValue( props().size_ );
    colfld_->setColor( props().color_ );
    doPutToScreen();
}


void uiWellDispProperties::getFromScreen()
{
    props().size_ = szfld_->getValue();
    props().color_ = colfld_->color();
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

    nmsizefld_ = new uiLabeledSpinBox( this, "Name size" );
    nmsizefld_->box()->setInterval(10,30,6);
    nmsizefld_->attach( alignedBelow, dispabovefld_  );

    doPutToScreen();

    dispabovefld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    dispbelowfld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    nmsizefld_->box()->valueChanging.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
}


void uiWellTrackDispProperties::resetProps( Well::DisplayProperties::Track& pp )
{
    props_ = &pp; 
}


void uiWellTrackDispProperties::doPutToScreen()
{
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


static const char* shapes[] = { "Cylinder", "Square", "Sphere", 0 };
uiWellMarkersDispProperties::uiWellMarkersDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Markers& mp )
    : uiWellDispProperties(p,su,mp)
{
    shapefld_ = new uiLabeledComboBox( this, "Shape" );
    shapefld_->attach( alignedBelow, colfld_ );
    for ( int idx=0; shapes[idx]; idx++)
	shapefld_->box()->addItem( shapes[idx] );
    
    cylinderheightfld_ = new uiLabeledSpinBox( this, "Height" );
    cylinderheightfld_->box()->setInterval( 0, 10, 1 );
    cylinderheightfld_->attach( rightOf, shapefld_ );
   
    singlecolfld_ = new uiCheckBox( this, "use single color");
    singlecolfld_ -> attach( rightOf, colfld_); 
    colfld_->setSensitive( singlecolfld_->isChecked() );
   
    nmsizefld_ = new uiLabeledSpinBox( this, "Names size" );
    nmsizefld_->box()->setInterval(10,30,6);
    nmsizefld_->attach( alignedBelow, shapefld_ );
    
    uiColorInput::Setup csu( mrkprops().color_ );
    BufferString dlgtxt( "Names color" );
    csu.lbltxt( dlgtxt ).withalpha( false );
    nmcolfld_ = new uiColorInput( this, csu, dlgtxt );
    nmcolfld_->attach( alignedBelow, nmsizefld_ );

    samecolasmarkerfld_ = new uiCheckBox( this, "same as markers");
    samecolasmarkerfld_->attach( rightOf, nmcolfld_); 
    
    doPutToScreen();

    cylinderheightfld_->box()->valueChanging.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    nmcolfld_->colorChanged.notify( 
		mCB(this,uiWellMarkersDispProperties,propChg) );
    nmsizefld_->box()->valueChanging.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    samecolasmarkerfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    samecolasmarkerfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,markerFldsChged));
    singlecolfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    singlecolfld_->activated.notify(
		mCB(this,uiWellMarkersDispProperties,markerFldsChged));
    shapefld_->box()->selectionChanged.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    shapefld_->box()->selectionChanged.notify(
		mCB(this,uiWellMarkersDispProperties,markerFldsChged));
}


void uiWellMarkersDispProperties::resetProps( Well::DisplayProperties::Markers& pp )
{
    props_ = &pp;
}


void uiWellMarkersDispProperties::markerFldsChged( CallBacker*  )
{
    colfld_->setSensitive( singlecolfld_->isChecked() );
    nmcolfld_->setSensitive( !samecolasmarkerfld_->isChecked() );
    cylinderheightfld_->display( !shapefld_->box()->currentItem() );
}


void uiWellMarkersDispProperties::doPutToScreen()
{
    NotifyStopper ns1( cylinderheightfld_->box()->valueChanging );

    shapefld_->box()->setCurrentItem( mrkprops().shapeint_ );
    cylinderheightfld_->box()->setValue( mrkprops().cylinderheight_ );
    singlecolfld_->setChecked( mrkprops().issinglecol_ );
    nmsizefld_->box()->setValue( mrkprops().nmsize_ );
    samecolasmarkerfld_->setChecked( mrkprops().samenmcol_ );
    nmcolfld_->setColor( mrkprops().nmcol_ );
}


void uiWellMarkersDispProperties::doGetFromScreen()
{
    mrkprops().shapeint_ = shapefld_->box()->currentItem();
    mrkprops().cylinderheight_ = cylinderheightfld_->box()->getValue();
    mrkprops().issinglecol_ = singlecolfld_->isChecked();
    mrkprops().nmsize_ =  nmsizefld_->box()->getValue();
    mrkprops().samenmcol_ = samecolasmarkerfld_->isChecked();
    mrkprops().nmcol_ =  nmcolfld_->color();
}


uiWellLogDispProperties::uiWellLogDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Log& lp, 
				const Well::LogSet* wl)
    : uiWellDispProperties(p,su,lp)
{
    stylefld_ = new uiGenInput( this, "Style", 
			        BoolInpSpec(true,"Well log","Seismic") );
    stylefld_->attach( alignedAbove, szfld_ );

    rangefld_ = new uiGenInput( this, "Log range (min/max)",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    rangefld_->attach( alignedAbove, stylefld_ );
   
    const char* choice[] = { "clip rate", "data range", 0 };
    cliprangefld_ = new uiGenInput( this, "Specify", StringListInpSpec(choice));
    cliprangefld_->attach( alignedAbove, rangefld_ );
   
    clipratefld_ = new uiGenInput( this, "Clip rate", StringInpSpec() );
    clipratefld_->setElemSzPol( uiObject::Small );
    clipratefld_->attach( alignedBelow, cliprangefld_ );

    logarithmfld_ = new uiCheckBox( this, "Logarithmic" );
    logarithmfld_->attach( rightOf, rangefld_ );
    
    revertlogfld_ = new uiCheckBox( this, "Revert" );
    revertlogfld_->attach( rightOf, cliprangefld_ );

    logfillfld_ = new uiCheckBox( this, "log filled" );
    logfillfld_->attach( rightOf, colfld_ );   

    singlfillcolfld_ = new uiCheckBox( this, "single color" );
    singlfillcolfld_->setName( BufferString("single color") );
    singlfillcolfld_->attach(rightOf, logfillfld_);

    BufferString sellbl( "Select log" );
    logsfld_ = new uiLabeledComboBox( this, sellbl );
    logsfld_->box()->setHSzPol( uiObject::Wide );
    logsfld_->attach( alignedAbove, cliprangefld_ );

    BufferString selfilllbl( "Log fill range (min/max)" );
    filllogsfld_ = new uiLabeledComboBox( this, selfilllbl );
    filllogsfld_->attach( alignedBelow, colfld_ );

    setLogSet( wl );

    coltablistfld_ = new uiColorTableSel( this, "Table selection" );
    coltablistfld_->attach( rightOf, filllogsfld_ );

    colorrangefld_ = new uiGenInput( this, "Color range",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    colorrangefld_->attach( alignedBelow, filllogsfld_ );

    logwidthfld_ = new uiLabeledSpinBox( this, "Log screen width" );
    logwidthfld_->box()->setInterval(1,500);
    logwidthfld_->attach( rightOf, szfld_ );

    seiscolorfld_ = new uiColorInput( this,
		                 uiColorInput::Setup(logprops().seiscolor_)
			        .lbltxt("Filling color") );
    seiscolorfld_->attach( alignedBelow, logwidthfld_);
    seiscolorfld_->display(false);

    fillcolorfld_ = new uiColorInput( this,
		                 uiColorInput::Setup(logprops().seiscolor_)
			        .lbltxt("Filling color") );
    fillcolorfld_->attach( alignedBelow, colfld_);
    fillcolorfld_->display(false);

    lblr_ = new uiLabeledSpinBox( this, "Repeat" );
    repeatfld_ = lblr_ ->box();
    repeatfld_->setInterval( 1, 20, 1 );
    lblr_->attach(alignedBelow, colfld_);
  
    lblo_ = new uiLabeledSpinBox( this, "Overlap" );
    ovlapfld_ = lblo_->box();
    ovlapfld_->setInterval( 0, 100, 20 );
    lblo_->attach( alignedBelow, seiscolorfld_ );
    
    recoverProp();

    CallBack propchgcb = mCB(this,uiWellLogDispProperties,propChg);
    CallBack choiceselcb = mCB(this,uiWellLogDispProperties,choiceSel);
    cliprangefld_->valuechanged.notify( choiceselcb );
    clipratefld_->valuechanged.notify( choiceselcb );
    colorrangefld_->valuechanged.notify( choiceselcb );
    rangefld_->valuechanged.notify( choiceselcb );
    clipratefld_->valuechanged.notify( propchgcb );
    coltablistfld_->selectionChanged.notify( propchgcb );
    colorrangefld_->valuechanged.notify( propchgcb );
    fillcolorfld_->colorChanged.notify( propchgcb );
    logfillfld_->activated.notify( propchgcb );
    logwidthfld_->box()->valueChanging.notify( propchgcb );
    logarithmfld_->activated.notify( propchgcb );
    ovlapfld_->valueChanging.notify( propchgcb );
    rangefld_->valuechanged.notify( propchgcb );
    repeatfld_->valueChanging.notify( propchgcb );
    revertlogfld_->activated.notify( propchgcb );
    seiscolorfld_->colorChanged.notify( propchgcb );
    singlfillcolfld_->activated.notify( propchgcb );
    stylefld_->valuechanged.notify( propchgcb );

    filllogsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );
    logfillfld_->activated.notify(
		mCB(this,uiWellLogDispProperties,isFilledSel));
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,logSel) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateRange) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );
    repeatfld_->valueChanging.notify(
		mCB(this,uiWellLogDispProperties,isRepeatSel) );
    singlfillcolfld_->activated.notify(
		mCB(this,uiWellLogDispProperties,isFilledSel) );
    stylefld_->valuechanged.notify(
		mCB(this,uiWellLogDispProperties,isSeismicSel) );
    stylefld_->valuechanged.notify(
		mCB(this,uiWellLogDispProperties,isStyleChanged) );
}


void uiWellLogDispProperties::resetProps( Well::DisplayProperties::Log& pp )
{
    props_ = &pp;
    recoverProp();
}


void uiWellLogDispProperties::doPutToScreen()
{
    NotifyStopper nssfc( singlfillcolfld_->activated );
    NotifyStopper nsslf( logfillfld_->activated );
    NotifyStopper nso( ovlapfld_->valueChanging );
    NotifyStopper nsr( repeatfld_->valueChanging );
    NotifyStopper nsl( logarithmfld_->activated );
    NotifyStopper nsrev( revertlogfld_->activated );
    
    revertlogfld_->setChecked( logprops().islogreverted_ ); 
    logsfld_->box()->setText( logprops().name_ );
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
    bool isreverted = revertlogfld_->isChecked();
    if ( !logprops().range_.isRev() && isreverted )
	logprops().range_.sort( false ); 
    if ( logprops().range_.isRev() && !isreverted )
	logprops().range_.sort( true ); 

    logprops().fillrange_ = colorrangefld_->getFInterval();
    logprops().islogreverted_ = revertlogfld_->isChecked();
    logprops().islogarithmic_ = logarithmfld_->isChecked(); 
    logprops().islogfill_ = logfillfld_->isChecked();
    logprops().issinglecol_ = singlfillcolfld_->isChecked();
    logprops().seqname_ = coltablistfld_-> text();
    logprops().repeat_ = stylefld_->getBoolValue() ? 1 : repeatfld_->getValue();
    logprops().repeatovlap_ = ovlapfld_->getValue();
    logprops().seiscolor_ = logprops().iswelllog_ ? fillcolorfld_->color() 
						  : seiscolorfld_->color();
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
    putToScreen();
    logSel(0);
    isSeismicSel(0);
    choiceSel(0);
    isFilledSel(0);
}


void uiWellLogDispProperties::setLogSet( const Well::LogSet* wls )
{
    wl_ = wls;
    BufferStringSet lognames;
    for ( int idx=0; idx< wl_->size(); idx++ )
	lognames.addIfNew( wl_->getLog(idx).name() );
    lognames.sort();
    logsfld_->box()->empty();
    logsfld_->box()->addItem("None");
    logsfld_->box()->addItems( lognames );
    filllogsfld_->box()->empty();
    filllogsfld_->box()->addItems( lognames );
}


void uiWellLogDispProperties::logSel( CallBacker* )
{
    setFieldVals();
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
    revertlogfld_->setSensitive( yn );
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


void uiWellLogDispProperties::setFieldVals()
{
    BufferString sel = logsfld_->box()->text();
    if ( sel == "None" || sel == "none" )
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


void uiWellLogDispProperties::disableSeisStyle( bool yn )
{
    stylefld_->display( !yn );
    seiscolorfld_->display( !yn );
    ovlapfld_->display( !yn ); 
    repeatfld_->display( !yn );
    stylefld_->setValue( yn );
}

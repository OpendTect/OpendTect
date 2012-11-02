/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelldispprop.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicolortable.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uiseparator.h"

#include "coltabsequence.h"
#include "multiid.h"
#include "pixmap.h"
#include "welldata.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"


static int deflogwidth = 30;


static const char* fontstyles[] =
{ "Normal", "Bold", "Italic", "Bold Italic", 0 };


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

    uiColorInput::Setup csu( props().color_ ); csu.lbltxt( su.mycoltxt_ );
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
    NotifyStopper ns1( szfld_->valueChanging );
    NotifyStopper ns2( colfld_->colorChanged );
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
    nmsizefld_->box()->setInterval(5,30,2);
    nmsizefld_->attach( alignedBelow, dispabovefld_  );

    nmstylefld_ = new uiComboBox( this, fontstyles, "Fontstyle" );
    nmstylefld_->attach( rightOf, nmsizefld_ );

    doPutToScreen();

    dispabovefld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    dispbelowfld_->activated.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    nmsizefld_->box()->valueChanging.notify(
		mCB(this,uiWellTrackDispProperties,propChg) );
    nmstylefld_->selectionChanged.notify(
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
    nmsizefld_->box()->setValue( trackprops().font_.pointSize() );

    int style = trackprops().font_.weight()>FontData::Normal ? 1 : 0;
    if ( trackprops().font_.isItalic() )
	style += 2;

    nmstylefld_->setValue( style );
}


void uiWellTrackDispProperties::doGetFromScreen()
{
    trackprops().dispbelow_ = dispbelowfld_->isChecked();
    trackprops().dispabove_ = dispabovefld_->isChecked();
    trackprops().font_.setPointSize( nmsizefld_->box()->getValue() );

    const int fontstyle = nmstylefld_->getIntValue();
    const bool bold = fontstyle==1 || fontstyle==3;
    trackprops().font_.setWeight( bold ? FontData::Bold : FontData::Normal );
    trackprops().font_.setItalic( fontstyle==2 || fontstyle==3 );
}


static const char* shapes3d[] = { "Cylinder", "Square", "Sphere", 0 };
static const char* shapes2d[] = { "Dot", "Solid", "Dash", 0 };
uiWellMarkersDispProperties::uiWellMarkersDispProperties( uiParent* p,
				const uiWellDispProperties::Setup& su,
				Well::DisplayProperties::Markers& mp,
				const BufferStringSet& allmarkernms, bool is2d )
    : uiWellDispProperties(p,su,mp)
    , is2d_(is2d)				     
{
    shapefld_ = new uiLabeledComboBox( this, "Shape" );
    shapefld_->attach( alignedBelow, colfld_ );
    for ( int idx=0; shapes3d[idx]; idx++)
	shapefld_->box()->addItem( is2d ? shapes2d[idx] : shapes3d[idx] );
    
    cylinderheightfld_ = new uiLabeledSpinBox( this, "Height" );
    cylinderheightfld_->box()->setInterval( 0, 10, 1 );
    cylinderheightfld_->attach( rightOf, shapefld_ );
    cylinderheightfld_->display( !is2d );
   
    singlecolfld_ = new uiCheckBox( this, "use single color");
    singlecolfld_->attach( rightOf, colfld_); 
    colfld_->setSensitive( singlecolfld_->isChecked() );
   
    nmsizefld_ = new uiLabeledSpinBox( this, "Names size" );
    nmsizefld_->box()->setInterval(5,30,2);
    nmsizefld_->attach( alignedBelow, shapefld_ );

    const char* styles[] = { "Normal", "Bold", "Italic", "Bold Italic", 0 };
    nmstylefld_ = new uiComboBox( this, styles, "Fontstyle" );
    nmstylefld_->attach( rightOf, nmsizefld_ );

    const char* dlgtxt = "Names color";
    uiColorInput::Setup csu( mrkprops().color_ ); csu.lbltxt( dlgtxt );
    nmcolfld_ = new uiColorInput( this, csu, dlgtxt );
    nmcolfld_->attach( alignedBelow, nmsizefld_ );

    samecolasmarkerfld_ = new uiCheckBox( this, "same as markers");
    samecolasmarkerfld_->attach( rightOf, nmcolfld_); 
   
    checkallfld_ = new uiCheckBox( this, "All" );
    checkallfld_->attach( alignedBelow, nmcolfld_ );
    checkallfld_->setChecked( true );

    uiLabel* lbl = new uiLabel( this, "Display markers" );
    lbl->attach( leftOf, checkallfld_  );

    displaymarkersfld_ = new uiListBox( this, lbl->text() );
    displaymarkersfld_->addItems( allmarkernms );
    displaymarkersfld_->setItemsCheckable( true );
    displaymarkersfld_->attach( alignedBelow, checkallfld_ ); 

    doPutToScreen();
    markerFldsChged(0);

    cylinderheightfld_->box()->valueChanging.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    nmcolfld_->colorChanged.notify( 
		mCB(this,uiWellMarkersDispProperties,propChg) );
    nmsizefld_->box()->valueChanging.notify(
		mCB(this,uiWellMarkersDispProperties,propChg) );
    nmstylefld_->selectionChanged.notify(
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
    checkallfld_->activated.notify(
		    mCB(this,uiWellMarkersDispProperties,markerFldsChged) );
    checkallfld_->activated.notify(
		    mCB(this,uiWellMarkersDispProperties,propChg) );
    displaymarkersfld_->itemChecked.notify(
	    		mCB(this,uiWellMarkersDispProperties,propChg) );
    displaymarkersfld_->itemChecked.notify(
	    		mCB(this,uiWellMarkersDispProperties,markerFldsChged) );
}


void uiWellMarkersDispProperties::getSelNames()
{
    mrkprops().selmarkernms_.erase();
    for ( int idx=0; idx<displaymarkersfld_->size(); idx++ )
    {
	if ( displaymarkersfld_->isItemChecked( idx ) )
	    mrkprops().selmarkernms_.add( displaymarkersfld_->textOfItem(idx) );
    }
}


void uiWellMarkersDispProperties::setSelNames()
{
    NotifyStopper ns( displaymarkersfld_->itemChecked );
    for ( int idx=0; idx<displaymarkersfld_->size(); idx ++ ) 
    {
	BufferString mrknm = displaymarkersfld_->textOfItem(idx);
	const bool ispresent = mrkprops().selmarkernms_.isPresent( mrknm );
	displaymarkersfld_->setItemChecked( idx, ispresent );
    }
}


void uiWellMarkersDispProperties::resetProps(
				Well::DisplayProperties::Markers& pp )
{
    props_ = &pp;
}


void uiWellMarkersDispProperties::markerFldsChged( CallBacker* cb )
{
    colfld_->setSensitive( singlecolfld_->isChecked() );
    nmcolfld_->setSensitive( !samecolasmarkerfld_->isChecked() );
    cylinderheightfld_->display( !shapefld_->box()->currentItem() && !is2d_ );

    mDynamicCastGet(uiCheckBox*,allfld,cb)
    if ( allfld )
    {
	displaymarkersfld_->itemChecked.disable();
	const bool ischecked = checkallfld_->isChecked();
	for ( int idx=0; idx<displaymarkersfld_->size(); idx++ )
	{
	    if ( displaymarkersfld_->isItemChecked(idx) != ischecked )
	    displaymarkersfld_->setItemChecked( idx, ischecked );
	}
	displaymarkersfld_->itemChecked.enable();
	return;
    }

    bool chkall = true;
    for ( int idx=0; idx<displaymarkersfld_->size(); idx++ )
    {
	if ( !displaymarkersfld_->isItemChecked(idx) )
	    { chkall = false; break; }
    }

    checkallfld_->activated.disable();
    checkallfld_->setChecked( chkall );
    checkallfld_->activated.enable();

    displaymarkersfld_->itemChecked.enable();
}


void uiWellMarkersDispProperties::doPutToScreen()
{
    NotifyStopper ns1( cylinderheightfld_->box()->valueChanging );

    shapefld_->box()->setCurrentItem( mrkprops().shapeint_ );
    cylinderheightfld_->box()->setValue( mrkprops().cylinderheight_ );
    singlecolfld_->setChecked( mrkprops().issinglecol_ );
    nmsizefld_->box()->setValue( mrkprops().font_.pointSize() );

    int style = mrkprops().font_.weight()>FontData::Normal ? 1 : 0;
    if ( mrkprops().font_.isItalic() )
	style += 2;

    nmstylefld_->setValue( style );

    samecolasmarkerfld_->setChecked( mrkprops().samenmcol_ );
    nmcolfld_->setColor( mrkprops().nmcol_ );
    setSelNames();
}


void uiWellMarkersDispProperties::doGetFromScreen()
{
    mrkprops().shapeint_ = shapefld_->box()->currentItem();
    mrkprops().cylinderheight_ = cylinderheightfld_->box()->getValue();
    mrkprops().issinglecol_ = singlecolfld_->isChecked();
    mrkprops().font_.setPointSize( nmsizefld_->box()->getValue() );
    const int fontstyle = nmstylefld_->getIntValue();
    const bool bold = fontstyle==1 || fontstyle==3;
    mrkprops().font_.setWeight( bold ? FontData::Bold : FontData::Normal );
    mrkprops().font_.setItalic( fontstyle==2 || fontstyle==3 );
    mrkprops().samenmcol_ = samecolasmarkerfld_->isChecked();
    mrkprops().nmcol_ = nmcolfld_->color();
    getSelNames();
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

    uiSeparator* sep1 = new uiSeparator( this, "Sep" ); 
    sep1->attach( stretchedAbove, stylefld_ );

    rangefld_ = new uiGenInput( this, "Log range (min/max)",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    rangefld_->attach( alignedAbove, stylefld_ );
    sep1->attach( stretchedBelow, rangefld_ );

    const char* choice[] = { "clip rate", "data range", 0 };
    cliprangefld_ = new uiGenInput( this, "Specify", StringListInpSpec(choice));
    cliprangefld_->attach( alignedAbove, rangefld_ );
   
    clipratefld_ = new uiGenInput( this, "Clip rate", StringInpSpec() );
    clipratefld_->setElemSzPol( uiObject::Small );
    clipratefld_->attach( alignedBelow, cliprangefld_ );

    logarithmfld_ = new uiCheckBox( this, "Logarithmic" );
    logarithmfld_->attach( rightOf, rangefld_ );
    
    revertlogfld_ = new uiCheckBox( this, "Flip" );
    revertlogfld_->attach( rightOf, cliprangefld_ );

    lblr_ = new uiLabeledSpinBox( this, "Repeat" );
    repeatfld_ = lblr_ ->box();
    repeatfld_->setInterval( 1, 20, 1 );
    lblr_->attach( alignedBelow, colfld_ );
  
    BufferString sellbl( "Select log" );
    logsfld_ = new uiLabeledComboBox( this, sellbl );
    logsfld_->box()->setHSzPol( uiObject::Wide );
    logsfld_->attach( alignedAbove, cliprangefld_ );

    logfilltypefld_ = new uiLabeledComboBox( this, "Fill ");
    logfilltypefld_->box()->addItem( "None" );
    logfilltypefld_->box()->addItem( "Left of log" );
    logfilltypefld_->box()->addItem( "Right of log" );
    logfilltypefld_->box()->addItem( "Full panel" );
    logfilltypefld_->attach( alignedBelow, colfld_ );

    BufferString selfilllbl( "Fill with " );
    filllogsfld_ = new uiLabeledComboBox( this, selfilllbl );
    filllogsfld_->attach( alignedBelow, logfilltypefld_ );

    singlfillcolfld_ = new uiCheckBox( this, "single color" );
    singlfillcolfld_->attach( rightOf, logfilltypefld_ );

    setLogSet( wl );

    coltablistfld_ = new uiColorTableSel( this, "Table selection" );
    coltablistfld_->attach( alignedBelow, filllogsfld_ );

    colorrangefld_ = new uiGenInput( this, "",
			     FloatInpIntervalSpec()
			     .setName(BufferString(" range start"),0)
			     .setName(BufferString(" range stop"),1) );
    colorrangefld_->attach( rightOf, coltablistfld_ );

    flipcoltabfld_ = new uiCheckBox( this, "flip color table" );
    flipcoltabfld_->attach( rightOf, filllogsfld_ );

    uiSeparator* sep2 = new uiSeparator( this, "Sep" ); 
    sep2->attach( stretchedBelow, coltablistfld_ );
    logwidthfld_ = new uiLabeledSpinBox( this, "Log screen width" );
    logwidthfld_->box()->setInterval(1,500);
    logwidthfld_->attach( alignedBelow, coltablistfld_ );
    logwidthfld_->attach( ensureBelow, sep2 );

    seiscolorfld_ = new uiColorInput( this,
		                 uiColorInput::Setup(logprops().seiscolor_)
			        .lbltxt("Filling color") );
    seiscolorfld_->attach( alignedBelow, lblr_ );
    seiscolorfld_->display(false);

    lblo_ = new uiLabeledSpinBox( this, "Overlap" );
    ovlapfld_ = lblo_->box();
    ovlapfld_->setInterval( 0, 100, 20 );
    lblo_->attach( rightOf, lblr_ );

    fillcolorfld_ = new uiColorInput( this,
		                 uiColorInput::Setup(logprops().seiscolor_)
			        .lbltxt("Filling color") );
    fillcolorfld_->attach( alignedBelow, logfilltypefld_ );
    fillcolorfld_->display(false);

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
    logwidthfld_->box()->valueChanging.notify( propchgcb );
    logarithmfld_->activated.notify( propchgcb );
    ovlapfld_->valueChanging.notify( propchgcb );
    rangefld_->valuechanged.notify( propchgcb );
    repeatfld_->valueChanging.notify( propchgcb );
    revertlogfld_->activated.notify( propchgcb );
    seiscolorfld_->colorChanged.notify( propchgcb );
    singlfillcolfld_->activated.notify( propchgcb );
    stylefld_->valuechanged.notify( propchgcb );
    logfilltypefld_->box()->selectionChanged.notify( propchgcb );
    flipcoltabfld_->activated.notify( propchgcb );

    filllogsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,logSel) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateRange) );
    logsfld_->box()->selectionChanged.notify(
		mCB(this,uiWellLogDispProperties,updateFillRange) );
    logfilltypefld_->box()->selectionChanged.notify( 
	    	mCB(this,uiWellLogDispProperties,isFilledSel));
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
}


#define mSetSwapFillIdx( fidx )\
        if ( logprops().islogreverted_ )\
	{ if ( fidx == 2 ) fidx = 1; else if ( fidx == 1 ) fidx =2; }
void uiWellLogDispProperties::doPutToScreen()
{
    NotifyStopper nssfc( singlfillcolfld_->activated );
    NotifyStopper nso( ovlapfld_->valueChanging );
    NotifyStopper nsr( repeatfld_->valueChanging );
    NotifyStopper nsl( logarithmfld_->activated );
    NotifyStopper nsrev( revertlogfld_->activated );
    NotifyStopper nsslf( logfilltypefld_->box()->selectionChanged );
    
    revertlogfld_->setChecked( logprops().islogreverted_ ); 
    logsfld_->box()->setText( logprops().name_ );
    rangefld_->setValue( logprops().range_ );
    colorrangefld_->setValue( logprops().fillrange_ );
    filllogsfld_->box()-> setText( logprops().fillname_ );
    stylefld_->setValue( logprops().iswelllog_ );
    logarithmfld_->setChecked( logprops().islogarithmic_ ); 
    coltablistfld_->setCurrent( logprops().seqname_ ); 
    flipcoltabfld_->setChecked( logprops().iscoltabflipped_ );
    ovlapfld_->setValue( logprops().repeatovlap_ );
    repeatfld_->setValue( logprops().repeat_ );
    int fidx = logprops().isleftfill_ ? logprops().isrightfill_ ? 3 : 1
				      : logprops().isrightfill_ ? 2 : 0; 
    mSetSwapFillIdx( fidx )

    logfilltypefld_->box()->setCurrentItem( fidx );
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
    logprops().issinglecol_ = singlfillcolfld_->isChecked();
    int fillidx = logfilltypefld_->box()->currentItem();
    mSetSwapFillIdx( fillidx )
    logprops().isleftfill_ = ( fillidx == 1 || fillidx == 3 );
    logprops().isrightfill_ = ( fillidx == 2 || fillidx == 3 );
    logprops().seqname_ = coltablistfld_->text();
    logprops().iscoltabflipped_ = flipcoltabfld_->isChecked();
    logprops().repeat_ = stylefld_->getBoolValue() ? 1 : repeatfld_->getValue();
    logprops().repeatovlap_ = mCast( float, ovlapfld_->getValue() );
    logprops().seiscolor_ = logprops().iswelllog_ ? fillcolorfld_->color() 
						  : seiscolorfld_->color();
    logprops().name_ = logsfld_->box()->text();
    logprops().fillname_ = filllogsfld_->box()->text();
    deflogwidth = logprops().logwidth_ = logwidthfld_->box()->getValue();
}


void uiWellLogDispProperties::isFilledSel( CallBacker* )
{
    const bool iswelllog = stylefld_->getBoolValue();
    const bool issinglecol = singlfillcolfld_->isChecked();
    const int fillidx = logfilltypefld_->box()->currentItem();
    const bool isleftfilled_ = fillidx == 1 || fillidx == 3;
    const bool isrightfilled_ = fillidx == 2 || fillidx == 3;
    const bool isfilled = isrightfilled_ || isleftfilled_;
    singlfillcolfld_->display( isfilled && iswelllog );
    coltablistfld_->display( iswelllog &&  isfilled && !issinglecol );
    seiscolorfld_->display( !iswelllog );
    fillcolorfld_->display( (issinglecol && isfilled) );
    filllogsfld_->display( iswelllog &&  isfilled && !issinglecol );
    colorrangefld_->display( iswelllog &&  isfilled && !issinglecol );
    flipcoltabfld_->display( isfilled && iswelllog && !issinglecol );
}


void uiWellLogDispProperties::isRepeatSel( CallBacker* )
{
    const bool iswelllog = stylefld_->getBoolValue();
    if ( iswelllog )
	repeatfld_-> setValue( 1 );
}


void uiWellLogDispProperties::isSeismicSel( CallBacker* )
{
    const bool iswelllog = stylefld_->getBoolValue();
    lblr_->display( !iswelllog );
    lblo_->display( !iswelllog );
    logfilltypefld_->display( iswelllog );
    flipcoltabfld_->display( iswelllog );
    revertlogfld_->display( iswelllog );
    if (iswelllog)
	repeatfld_->setValue(1);
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
    flipcoltabfld_->display( iswelllog );
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
    logsfld_->box()->setEmpty();
    logsfld_->box()->addItem("None");
    logsfld_->box()->addItems( lognames );
    filllogsfld_->box()->setEmpty();
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
    colfld_->setColor( logprops().color_ );
    seiscolorfld_->setColor( logprops().seiscolor_ );
    fillcolorfld_->setColor( logprops().seiscolor_ );
    stylefld_->setValue( true );
    setFldSensitive( false );
    cliprangefld_->setValue( true );
    clipratefld_->setValue( 0.0 );
    repeatfld_->setValue( 0 );
    ovlapfld_->setValue( 0 );
    singlfillcolfld_->setChecked( false );
    coltablistfld_->setCurrent( logprops().seqname_ ); 
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
    szfld_->setSensitive( yn );
    singlfillcolfld_->setSensitive( yn );
    coltablistfld_->setSensitive( yn );
    filllogsfld_->setSensitive(yn);
    logarithmfld_->setSensitive(yn);
    logwidthfld_->setSensitive(yn);
    logfilltypefld_->setSensitive(yn);
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


void uiWellLogDispProperties::disableLogWidth( bool yn )
{
    logwidthfld_->display( !yn );
}

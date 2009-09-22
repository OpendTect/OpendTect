/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorizontracksetup.cc,v 1.33 2009-09-22 08:08:20 cvsumesh Exp $";

#include "uihorizontracksetup.h"

#include "attribdescset.h"
#include "attribsel.h"
#include "draw.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "horizonadjuster.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "randcolor.h"
#include "sectiontracker.h"
#include "separstr.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uislider.h"
#include "uitable.h"
#include "uitabstack.h"


#define mErrRet(s) { uiMSG().error( s ); return false; }

namespace MPE
{

void uiHorizonSetupGroup::initClass()
{
    uiMPE().setupgrpfact.addFactory( uiHorizonSetupGroup::create );
}


uiSetupGroup* uiHorizonSetupGroup::create( uiParent* p, const char* typestr,
					   const Attrib::DescSet* ads )
{
    if ( strcmp(typestr,EM::Horizon3D::typeStr()) && 
	 strcmp(typestr,EM::Horizon2D::typeStr()) )
	return 0;

    return new uiHorizonSetupGroup( p, ads, typestr );
}


const char** uiHorizonSetupGroup::sKeyEventNames()
{
    static const char* event_names[] = { "Min", "Max", "0+-", "0-+", 0 };
    return event_names;
}


const VSEvent::Type* uiHorizonSetupGroup::cEventTypes()
{
    static const VSEvent::Type event_types[] = { VSEvent::Min, VSEvent::Max,
					 VSEvent::ZCPosNeg, VSEvent::ZCNegPos };

    return event_types;
}


uiHorizonSetupGroup::uiHorizonSetupGroup( uiParent* p,
					  const Attrib::DescSet* ads,
       					  const char* typestr )
    : uiSetupGroup(p,"")
    , sectiontracker_(0)
    , attrset_(ads)
    , inpfld(0)
    , typestr_(typestr)
    , modechanged_(this)
    , eventchanged_(this)
    , similartychanged_(this)
    , propertychanged_(this)
{
    tabgrp_ = new uiTabStack( this, "TabStack" );
    uiGroup* modegrp = createModeGroup();
    tabgrp_->addTab( modegrp, "Mode" );

    uiGroup* eventgrp = createEventGroup();
    tabgrp_->addTab( eventgrp, "Event" );

    uiGroup* simigrp = createSimiGroup();
    tabgrp_->addTab( simigrp, "Similarity" );

    uiGroup* propertiesgrp = createPropertyGroup();
    tabgrp_->addTab( propertiesgrp, "Properties" );
    inwizard_ = p && !strncmp(p->name(),"Page",4);
}


uiGroup* uiHorizonSetupGroup::createModeGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Mode" );

    modeselgrp_ = new uiButtonGroup( grp, "ModeSel" );
    modeselgrp_->setExclusive( true );
    grp->setHAlignObj( modeselgrp_ );

    if ( typestr_ == EMHorizon3DTranslatorGroup::keyword() &&
	 Horizon3DSeedPicker::nrSeedConnectModes()>0 )
    {
	for ( int idx=0; idx<Horizon3DSeedPicker::nrSeedConnectModes(); idx++ )
	{
	    uiRadioButton* butptr = new uiRadioButton( modeselgrp_,
			Horizon3DSeedPicker::seedConModeText(idx,false) );
	    butptr->activated.notify( 
		    	mCB(this,uiHorizonSetupGroup,seedModeChange) );

	    mode_ = (EMSeedPicker::SeedModeOrder)
				Horizon3DSeedPicker::defaultSeedConMode();
	}
    }
    else if ( typestr_ == EMHorizon2DTranslatorGroup::keyword() &&
	      Horizon2DSeedPicker::nrSeedConnectModes()>0 )
    {
	for ( int idx=0; idx<Horizon2DSeedPicker::nrSeedConnectModes(); idx++ )
	{
	    uiRadioButton* butptr = new uiRadioButton( modeselgrp_,
		    	Horizon2DSeedPicker::seedConModeText(idx,false) );
	    butptr->activated.notify(
		    	mCB(this,uiHorizonSetupGroup,seedModeChange) );

	    mode_ = (EMSeedPicker::SeedModeOrder)
				Horizon2DSeedPicker::defaultSeedConMode();
	}
    }

    return grp;
}


uiGroup* uiHorizonSetupGroup::createEventGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Event" );

    if ( attrset_ )
	inpfld = new uiAttrSel( grp, *attrset_, "Input data" );
    else
	inpfld = new uiAttrSel( grp, "Input data", uiAttrSelData(false) );
    grp->setHAlignObj( inpfld );
    inpfld->selectiondone.notify( mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    evfld = new uiGenInput( grp, "Event type",
	    		    StringListInpSpec(sKeyEventNames()) );
    evfld->attach( alignedBelow, inpfld );
    evfld->valuechanged.notify( mCB(this,uiHorizonSetupGroup,selEventType) );
    evfld->valuechanged.notify( mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    BufferString srchwindtxt( "Search window " );
    srchwindtxt += SI().getZUnitString();
    srchgatefld = new uiGenInput( grp, srchwindtxt, FloatInpIntervalSpec() );
    srchgatefld->attach( alignedBelow, evfld );
    srchgatefld->valuechanged.notify( 
	    mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    thresholdtypefld = new uiGenInput( grp, "Threshold type",
		BoolInpSpec(true,"Cut-off amplitude","Relative difference") );
    thresholdtypefld->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,selAmpThresholdType) );
    thresholdtypefld->attach( alignedBelow, srchgatefld );

    ampthresholdfld = new uiGenInput ( grp, "Allowed difference (%)",
	    			       StringInpSpec() );
    ampthresholdfld->attach( alignedBelow, thresholdtypefld );
    ampthresholdfld->valuechanged.notify( 
	    mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    addstepbut = new uiPushButton( grp, "Steps",
	    mCB(this,uiHorizonSetupGroup,addStepPushedCB), false );
    addstepbut->attach( rightTo, ampthresholdfld );

    extriffailfld = new uiGenInput( grp, "If tracking fails",
				    BoolInpSpec(true,"Extrapolate","Stop") );
    extriffailfld->attach( alignedBelow, ampthresholdfld );
    extriffailfld->valuechanged.notify( 
	    mCB(this,uiHorizonSetupGroup,eventChangeCB) );

    return grp;
}


uiGroup* uiHorizonSetupGroup::createSimiGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Similarity" );

    usesimifld = new uiGenInput( grp, "Use similarity", BoolInpSpec(true) );
    usesimifld->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup,selUseSimilarity) );
    usesimifld->valuechanged.notify( 
	    mCB(this,uiHorizonSetupGroup, similartyChangeCB) );

    BufferString compwindtxt( "Compare window " );
    compwindtxt += SI().getZUnitString();
    compwinfld = new uiGenInput( grp, compwindtxt, FloatInpIntervalSpec() );
    compwinfld->attach( alignedBelow, usesimifld );
    compwinfld->valuechanged.notify( 
	    mCB(this,uiHorizonSetupGroup, similartyChangeCB) );

    simithresholdfld = new uiGenInput( grp, "Similarity threshold(0-1)",
				       FloatInpSpec() );
    simithresholdfld->attach( alignedBelow, compwinfld );
    simithresholdfld->valuechanged.notify(
	    mCB(this,uiHorizonSetupGroup, similartyChangeCB) );

    grp->setHAlignObj( usesimifld );
    return grp;
}


uiGroup* uiHorizonSetupGroup::createPropertyGroup()
{
    uiGroup* grp = new uiGroup( tabgrp_->tabGroup(), "Properties" );
    colorfld_ = new uiColorInput( grp, 
	    			  uiColorInput::Setup(getRandStdDrawColor() ).
				  lbltxt("Horizon color") );
    colorfld_->colorchanged.notify( 
	    		mCB(this,uiHorizonSetupGroup,colorChangeCB) );
    grp->setHAlignObj( colorfld_ );

    uiSeparator* sep = new uiSeparator( grp );
    sep->attach( stretchedBelow, colorfld_, -2 );

    seedtypefld_ = new uiGenInput( grp, "Seed Shape",
	    		StringListInpSpec(MarkerStyle3D::TypeNames()) );
    seedtypefld_->valuechanged.notify( 
	    		mCB(this,uiHorizonSetupGroup,seedTypeSel) );
    seedtypefld_->attach( alignedBelow, colorfld_ );
    seedtypefld_->attach( ensureBelow, sep );

    seedsliderfld_ = new uiSliderExtra( grp,
	    			uiSliderExtra::Setup("Seed Size").
				withedit(true),	"Slider Size" );
    seedsliderfld_->sldr()->setMinValue( 1 );
    seedsliderfld_->sldr()->setMaxValue( 15 );
    seedsliderfld_->sldr()->valueChanged.notify(
	    		mCB(this,uiHorizonSetupGroup,seedSliderMove));
    seedsliderfld_->attach( alignedBelow, seedtypefld_ );

    seedcolselfld_ = new uiColorInput( grp,
	    			       uiColorInput::Setup(Color::White()).
				       lbltxt("Seed Color") );
    seedcolselfld_->attach( alignedBelow, seedsliderfld_ );
    seedcolselfld_->colorchanged.notify( 
	    			mCB(this,uiHorizonSetupGroup,seedColSel) );

    return grp;
}


uiHorizonSetupGroup::~uiHorizonSetupGroup()
{
}


void uiHorizonSetupGroup::selUseSimilarity( CallBacker* )
{
    const bool usesimi = usesimifld->getBoolValue();
    compwinfld->display( usesimi );
    simithresholdfld->display( usesimi );
}


void uiHorizonSetupGroup::selAmpThresholdType( CallBacker* )
{
    const bool absthreshold = thresholdtypefld->getBoolValue();
    ampthresholdfld->setTitleText( absthreshold ? "       Amplitude value"
						: "Allowed difference (%)" );
    if ( absthreshold )
    {
	if (  horadj_->getAmplitudeThresholds().size() <=0 )
	    ampthresholdfld->setValue( horadj_->amplitudeThreshold() );
	else
	{
	    BufferString bs;
	    bs += horadj_->getAmplitudeThresholds()[0];
	    for (int idx=1;idx<horadj_->getAmplitudeThresholds().size();idx++)
	    { bs += ","; bs += horadj_->getAmplitudeThresholds()[idx]; }
	    ampthresholdfld->setText( bs.buf() );
	}
    }
    else
    {
	if ( horadj_->getAllowedVariances().size() <= 0 )
	    ampthresholdfld->setValue( horadj_->allowedVariance()*100 );
	else
	{
	    BufferString bs;
	    bs += horadj_->getAllowedVariances()[0]*100;
	    for ( int idx=1; idx<horadj_->getAllowedVariances().size(); idx++ )
	    { bs += ","; bs += horadj_->getAllowedVariances()[idx]*100; }
	    ampthresholdfld->setText( bs.buf() );
	}
    }
}


void uiHorizonSetupGroup::selEventType( CallBacker* )
{
    const VSEvent::Type ev = cEventTypes()[ evfld->getIntValue() ];
    const bool thresholdneeded = ev==VSEvent::Min || ev==VSEvent::Max;
    thresholdtypefld->setSensitive( thresholdneeded );
    ampthresholdfld->setSensitive( thresholdneeded );
}


void uiHorizonSetupGroup::seedModeChange( CallBacker* )
{
    mode_ = (EMSeedPicker::SeedModeOrder) modeselgrp_->selectedId();
    modechanged_.trigger();
}


void uiHorizonSetupGroup::eventChangeCB( CallBacker* )
{
    eventchanged_.trigger();
}


void uiHorizonSetupGroup::similartyChangeCB( CallBacker* )
{
    similartychanged_.trigger();
}


void uiHorizonSetupGroup::colorChangeCB( CallBacker* )
{
    propertychanged_.trigger();
}


void uiHorizonSetupGroup::seedTypeSel( CallBacker* )
{
    const MarkerStyle3D::Type newtype =
	(MarkerStyle3D::Type) (MarkerStyle3D::None+seedtypefld_->getIntValue());
    if ( markerstyle_.type_ == newtype )
	return;
    markerstyle_.type_ = newtype;
    propertychanged_.trigger();
}


void uiHorizonSetupGroup::seedSliderMove( CallBacker* )
{
    const float sldrval = seedsliderfld_->sldr()->getValue();
    const int newsize = mNINT(sldrval);
    if ( markerstyle_.size_ == newsize )
	return;
    markerstyle_.size_ = newsize;
    propertychanged_.trigger();
}


void uiHorizonSetupGroup::seedColSel( CallBacker* )
{
    const Color newcolor = seedcolselfld_->color();
    if ( markerstyle_.color_ == newcolor )
	return;
    markerstyle_.color_ = newcolor;
    propertychanged_.trigger();
}


class uiStepDialog : public uiDialog
{
public:

uiStepDialog( uiParent* p, const char* valstr )
    : uiDialog(p,Setup("Stepwise tracking","","108.0.1"))
{
    steptable_ = new uiTable( this, uiTable::Setup(5,1).rowdesc("Step")
				    .rowgrow(true).defrowlbl(true),
			      "Stepwise tracking table" );
    steptable_->setColumnLabel( 0, "Value" );

    SeparString ss( valstr, ',' );
    if ( ss.size() > 3 )
	steptable_->setNrRows( ss.size() + 2 );

    for ( int idx=0; idx<ss.size(); idx++ )
	steptable_->setText( RowCol(idx,0), ss[idx] );
}


void getValueString( BufferString& valstr )
{
    SeparString ss( 0, ',' );
    for ( int idx=0; idx<steptable_->nrRows(); idx++ )
    {
	const char* valtxt = steptable_->text( RowCol(idx,0) );
	if ( !valtxt || !*valtxt ) continue;
	ss.add( valtxt );
    }

    valstr = ss.buf();
}

    uiTable*	steptable_;
};


void uiHorizonSetupGroup::addStepPushedCB(CallBacker*)
{
    uiStepDialog dlg( this, ampthresholdfld->text() );
    if ( dlg.go() )
    {
	BufferString valstr;
	dlg.getValueString( valstr );
	ampthresholdfld->setText( valstr );
	propertychanged_.trigger();
    }
}
    

void uiHorizonSetupGroup::setSectionTracker( SectionTracker* st )
{
    sectiontracker_ = st;
    mDynamicCastGet(HorizonAdjuster*,horadj,sectiontracker_->adjuster())
    horadj_ = horadj;
    if ( !horadj_ ) return;

    initModeGroup();
    initEventGroup();
    selEventType(0);
    selAmpThresholdType(0);
    initSimiGroup();
    selUseSimilarity(0);
    initPropertyGroup();
}


void uiHorizonSetupGroup::setAttribSet( const Attrib::DescSet* ads )
{ 
    attrset_ = ads; 
    if ( inpfld )
	inpfld->setDescSet( ads );
}


void uiHorizonSetupGroup::initModeGroup()
{
    if ( typestr_ == EMHorizon3DTranslatorGroup::keyword() &&
	    Horizon3DSeedPicker::nrSeedConnectModes()>0 )
	modeselgrp_->selectButton( mode_ );
    
    else if ( typestr_ == EMHorizon2DTranslatorGroup::keyword() &&
	    Horizon2DSeedPicker::nrSeedConnectModes()>0 )
	modeselgrp_->selectButton( mode_ );
}



void uiHorizonSetupGroup::initEventGroup()
{
    Attrib::DescID curid = horadj_->getAttributeSel(0) ?
	horadj_->getAttributeSel(0)->id() : Attrib::DescID::undef();
    if ( attrset_ && attrset_->getDesc(curid) )
	inpfld->setDesc( attrset_->getDesc(curid) );

    VSEvent::Type ev = horadj_->trackEvent();
    const int fldidx = ev == VSEvent::Min ? 0
			    : (ev == VSEvent::Max ? 1
			    : (ev == VSEvent::ZCPosNeg ? 2 : 3) );
    evfld->setValue( fldidx );

    Interval<float> srchintv(
	    horadj_->permittedZRange().start * SI().zFactor(),
	    horadj_->permittedZRange().stop * SI().zFactor() );

    srchgatefld->setText( getStringFromFloat("%.5f",srchintv.start), 0 );
    srchgatefld->setText( getStringFromFloat("%.5f",srchintv.stop), 1 );

    thresholdtypefld->setValue( horadj_->useAbsThreshold() );
    extriffailfld->setValue( !horadj_->removesOnFailure() );
}


void uiHorizonSetupGroup::initSimiGroup()
{
    usesimifld->setValue( !horadj_->trackByValue() );

    Interval<float> simiintv(
	    horadj_->similarityWindow().start * SI().zFactor(),
	    horadj_->similarityWindow().stop * SI().zFactor() );

    compwinfld->setText( getStringFromFloat("%.5f",simiintv.start), 0 );
    compwinfld->setText( getStringFromFloat("%.5f",simiintv.stop), 1 );

    simithresholdfld->setValue( horadj_->similarityThreshold() );
}


void uiHorizonSetupGroup::initPropertyGroup()
{
    seedsliderfld_->sldr()->setValue( markerstyle_.size_ );
    seedcolselfld_->setColor( markerstyle_.color_ );
    seedtypefld_->setValue( markerstyle_.type_ - MarkerStyle3D::None );
}


void uiHorizonSetupGroup::setMode(EMSeedPicker::SeedModeOrder mode)
{
    mode_ = mode;
    modeselgrp_->selectButton( mode_ );
}


const int uiHorizonSetupGroup::getMode()
{
    return modeselgrp_ ? modeselgrp_->selectedId() : -1;
}


void uiHorizonSetupGroup::setColor( const Color& col)
{
    colorfld_->setColor( col );
}

const Color& uiHorizonSetupGroup::getColor()
{
    return colorfld_->color();
}


void uiHorizonSetupGroup::setMarkerStyle( const MarkerStyle3D& markerstyle )
{
    markerstyle_ = markerstyle;
    initPropertyGroup();
}


const MarkerStyle3D& uiHorizonSetupGroup::getMarkerStyle()
{
    return markerstyle_;
}


void uiHorizonSetupGroup::setAttribSelSpec( const Attrib::SelSpec* selspec )
{ inpfld->setSelSpec( selspec ); }


bool uiHorizonSetupGroup::isSameSelSpec( const Attrib::SelSpec* selspec ) const
{ return ( inpfld->attribID() == selspec->id() ); }


bool uiHorizonSetupGroup::commitToTracker( bool& fieldchange ) const
{
    fieldchange = false;

    if ( !horadj_ || horadj_->getNrAttributes()<1 )
    {   uiMSG().warning( "Unable to apply tracking setup" ); 
	return true;
    }
    if ( !inpfld ) return true;

    VSEvent::Type evtyp = cEventTypes()[ evfld->getIntValue() ];
    if ( horadj_->trackEvent() != evtyp )
    {
	fieldchange = true;
	horadj_->setTrackEvent( evtyp );
    }

    Interval<float> intv = srchgatefld->getFInterval();
    if ( intv.start>0 || intv.stop<0 || intv.start==intv.stop )
	mErrRet( "Search window should be minus to positive, ex. -20, 20");
    Interval<float> relintv( (float)intv.start/SI().zFactor(),
			     (float)intv.stop/SI().zFactor() );
    if ( horadj_->permittedZRange() != relintv )
    {
	fieldchange = true;
	horadj_->setPermittedZRange( relintv );
    }

    const bool usesimi = usesimifld->getBoolValue();
    if ( horadj_->trackByValue() == usesimi )
    {
	fieldchange = true;
	horadj_->setTrackByValue( !usesimi );
    }

    if ( usesimi )
    {
	Interval<float> intval = compwinfld->getFInterval();
	if ( intval.start>0 || intval.stop<0 || intval.start==intval.stop )
	    mErrRet( "Compare window should be minus to positive, ex. -20, 20");
	Interval<float> relintval( (float)intval.start/SI().zFactor(),
				   (float)intval.stop/SI().zFactor() );
	if ( horadj_->similarityWindow() != relintval )
	{
	    fieldchange = true;
	    horadj_->setSimilarityWindow( relintval );
	}
	    
	float mgate = simithresholdfld->getfValue();
	if ( mgate > 1 || mgate <= 0)
	    mErrRet( "Similarity threshold must be within 0 to 1" );
	if ( horadj_->similarityThreshold() != mgate )
	{
	    fieldchange = true;
	    horadj_->setSimilarityThreshold( mgate );
	}
    }
	    
    const bool useabs = thresholdtypefld->getBoolValue();
    if ( horadj_->useAbsThreshold() != useabs )
    {
	fieldchange = true;
	horadj_->setUseAbsThreshold( useabs );
    }

    if ( useabs )
    {
	SeparString ss( ampthresholdfld->text(), ',' );
	int idx = 0;
	if ( ss.size() < 2 )
	{
	    float vgate = ss.getFValue(0);
	    if ( Values::isUdf(vgate) )
		mErrRet( "Value threshold not set" );
	    if ( horadj_->amplitudeThreshold() != vgate )
	    {
		fieldchange = true;
		horadj_->setAmplitudeThreshold( vgate );
	    }
	}
	else
	{
	    TypeSet<float> vars;
	    for ( ; idx<ss.size(); idx++ )
	    {
		float varvalue = ss.getFValue(idx);
		if ( Values::isUdf(varvalue) )
		    mErrRet( "Value threshold not set properly" );

		if ( horadj_->getAmplitudeThresholds().size() < idx+1 )
		{
		    fieldchange = true;
		    horadj_->getAmplitudeThresholds() += varvalue;
		    if ( idx == 0 )
			horadj_->setAmplitudeThreshold( varvalue );
		}
		else if ( horadj_->getAmplitudeThresholds().size() >= idx+1 )
		    if ( horadj_->getAmplitudeThresholds()[idx] != varvalue )
		    {
			fieldchange = true;
			horadj_->getAmplitudeThresholds() += varvalue;
			if ( idx == 0 )
			    horadj_->setAmplitudeThreshold( varvalue );
		    }
	    }
	}

	if ( idx==0 && horadj_->getAmplitudeThresholds().size() > 0 )
	{
	    horadj_->getAmplitudeThresholds()[idx] =
				horadj_->amplitudeThreshold();
	    idx++;
	}

	if ( horadj_->getAmplitudeThresholds().size() > idx )
	{
	    int size = horadj_->getAmplitudeThresholds().size();
	    fieldchange = true;
	    horadj_->getAmplitudeThresholds().remove( idx, size-1 );
	}
    }
    else
    {
	SeparString ss( ampthresholdfld->text(), ',' );
	int idx = 0;
	if ( ss.size() < 2 )
	{
	    float var = ss.getFValue(0) / 100;
	    if ( var<=0.0 || var>=1.0 )
		mErrRet( "Allowed variance must be between 0-100" );
	    if ( horadj_->allowedVariance() != var )
	    {
		fieldchange = true;
		horadj_->setAllowedVariance( var );
	    }
	}
	else
	{
	    TypeSet<float> vars;
	    for ( ; idx<ss.size(); idx++ )
	    {
		float varvalue = ss.getFValue(idx) / 100;
		if ( varvalue <=0.0 || varvalue>=1.0 )
		    mErrRet( "Allowed variance must be between 0-100" );

		if ( horadj_->getAllowedVariances().size() < idx+1 )
		{
		    fieldchange = true;
		    horadj_->getAllowedVariances() += varvalue;
		    if ( idx == 0 )
			horadj_->setAllowedVariance( varvalue );
		}
		else if ( horadj_->getAllowedVariances().size() >= idx+1 )
		    if ( horadj_->getAllowedVariances()[idx] != varvalue )
		    {
			fieldchange = true;
			horadj_->getAllowedVariances()[idx] = varvalue;
			if ( idx == 0 )
			    horadj_->setAllowedVariance( varvalue );
		    }
	    }
	}

	if ( idx==0 && horadj_->getAllowedVariances().size()>0 )
	{
	    horadj_->getAllowedVariances()[idx] = horadj_->allowedVariance();
	    idx++;
	}

	if (  horadj_->getAllowedVariances().size() > idx )
	{
	    int size = horadj_->getAllowedVariances().size();
	    fieldchange = true;
	    horadj_->getAllowedVariances().remove( idx, size-1 );
	}
    }

    const bool rmonfail = !extriffailfld->getBoolValue();
    if ( horadj_->removesOnFailure() != rmonfail )    
    {
	fieldchange = true;
	horadj_->removeOnFailure( rmonfail );
    }

    inpfld->processInput();
    Attrib::SelSpec as;
    inpfld->fillSelSpec( as );
    if ( !as.id().isValid() )
	mErrRet( "Please select the seismic data to track on" );

    if ( !horadj_->getAttributeSel(0) || *horadj_->getAttributeSel(0)!=as )
    {
	fieldchange = true;
	horadj_->setAttributeSel( 0, as );
    }
    
    return true;
}


} //namespace MPE

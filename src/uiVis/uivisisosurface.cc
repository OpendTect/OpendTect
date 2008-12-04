/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		July 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisisosurface.cc,v 1.15 2008-12-04 17:31:26 cvsyuancheng Exp $";

#include "uivisisosurface.h"

#include "array3dfloodfill.h"
#include "attribdatacubes.h"
#include "marchingcubes.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "picksettr.h"
#include "pickset.h"
#include "survinfo.h"
#include "uiaxishandler.h"
#include "uibutton.h"
#include "uifunctiondisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistatsdisplay.h"
#include "viscolortab.h"
#include "vismarchingcubessurface.h"
#include "visvolumedisplay.h"


uiVisIsoSurfaceThresholdDlg::uiVisIsoSurfaceThresholdDlg( uiParent* p,
	visBase::MarchingCubesSurface* isosurface,
	visSurvey::VolumeDisplay* vd, bool couldchoosemode )
    : uiDlgGroup( p, "Iso surface threshold" )
    , isosurfacedisplay_( isosurface )
    , initialvalue_( vd->isoValue( isosurface ) )
    , vd_( vd )
    , usemode_( couldchoosemode )	       
{
    modefld_ = new uiGenInput( this, "Mode",
	    BoolInpSpec( true, "Full volume", "Seed based" ) );
    modefld_->setValue( true );
    modefld_->display( usemode_ );
    modefld_->valuechanged.notify( 
	    mCB(this,uiVisIsoSurfaceThresholdDlg,modeChangeCB) );
    
    ioobjselfld_ = new uiIOObjSel( this, *mMkCtxtIOObj(PickSet), "Seeds" );
    ioobjselfld_->setForRead( true );
    ioobjselfld_->display( false );
    ioobjselfld_->attach( alignedBelow, modefld_ );

    aboveisovaluefld_ = new uiGenInput( this, "Seeds value",
	    BoolInpSpec( true, "Above iso-value", "Below  iso-value" ) );
    aboveisovaluefld_->setValue( true );
    aboveisovaluefld_->display( false );
    aboveisovaluefld_->attach( alignedBelow, ioobjselfld_ );
    
    TypeSet<float> histogram;
    if ( vd->getHistogram(0) ) histogram = *vd->getHistogram(0);
    const Interval<float> rg = vd->getColorTab().getInterval();

    uiStatsDisplay::Setup su; su.withtext(false);
    statsdisplay_ = new uiStatsDisplay( this, su );
    statsdisplay_->setHistogram( histogram, rg );
    statsdisplay_->attach( alignedBelow, aboveisovaluefld_ );
    
    funcDisp().scene().getMouseEventHandler().buttonPressed.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,mousePressed) );
    funcDisp().scene().getMouseEventHandler().doubleClick.notify(
	    mCB( this, uiVisIsoSurfaceThresholdDlg,doubleClick) );

    thresholdfld_ = new uiGenInput( this, "Iso value",
	    FloatInpSpec(initialvalue_) );
    thresholdfld_->attach( alignedBelow, statsdisplay_ );
    updatebutton_ = new uiPushButton( this, "Update",
	    mCB( this, uiVisIsoSurfaceThresholdDlg, updatePressed ), false );
    updatebutton_->attach( rightOf, thresholdfld_ );
    drawHistogram();
}


uiVisIsoSurfaceThresholdDlg::~uiVisIsoSurfaceThresholdDlg()
{
}


bool uiVisIsoSurfaceThresholdDlg::acceptOK()
{
    const float curval = vd_->isoValue( isosurfacedisplay_ );
    const float fldvalue = thresholdfld_->getfValue();
    const float prec = (curval+fldvalue)/2000;
    const float defval = vd_->defaultIsoValue();
    
    if ( !mIsZero(curval-defval,defval/1000) && mIsZero(curval-fldvalue,prec) )
   	return true;

    if ( !usemode_ || (usemode_ && modefld_->getBoolValue()) )
    {
	updateIsoDisplay( fldvalue );
	return true;
    }

    if ( !ioobjselfld_->ctxtIOObj().ioobj )
    {
	uiMSG().error("No pickset selected");
	return false;
    }

    MultiID picksmid = ioobjselfld_->ctxtIOObj().ioobj->key();

    Pick::Set pickset;
    if ( Pick::Mgr().indexOf(picksmid)!=-1 )
	pickset = Pick::Mgr().get( picksmid );
    else
    {
	BufferString errmsg;
	if ( !PickSetTranslator::retrieve( pickset,
		    ioobjselfld_->ctxtIOObj().ioobj, errmsg ) )
	{
	    uiMSG().error( errmsg.buf() );
	    return false;
	}
    }

    const Array3D<float>& data = vd_->getCacheVolume(0)->getCube(0);
    if ( !data.isOK() )
    {
	uiMSG().error("Volume data is invalid.");
	return false;
    }

    Array3DImpl<float> newarr( data.info() );

    const bool aboveisoval = aboveisovaluefld_->getBoolValue();
    Array3DFloodfill<float> ff( data, fldvalue, aboveisoval, newarr );    
    ff.setOutsideValue( 1e+7 );
    
    const CubeSampling& cs = vd_->getCubeSampling( 0 );
    for ( int idx=0; idx<pickset.size(); idx++ )
    {
	const Coord3 pos =  pickset[idx].pos;
	const BinID bid = SI().transform( pos );
	const int i = cs.inlIdx( bid.inl );
	const int j = cs.crlIdx( bid.crl );
	const int k = cs.zIdx( pos.z );
	ff.addSeed( i, j, k );
    }
    
    if ( !ff.execute() )
	return false;

    return vd_->resetIsoSurface( isosurfacedisplay_, fldvalue, newarr );
}


bool uiVisIsoSurfaceThresholdDlg::rejectOK()
{
    return revertChanges();
}


bool uiVisIsoSurfaceThresholdDlg::revertChanges()
{
    const float curvalue = vd_->isoValue( isosurfacedisplay_ );
    const float prec = (curvalue+initialvalue_)/2000;
    if ( !mIsEqual(curvalue,initialvalue_,prec) )
	updateIsoDisplay(initialvalue_);

    return true;
}


void uiVisIsoSurfaceThresholdDlg::updatePressed(CallBacker*)
{
    updateIsoDisplay( thresholdfld_->getfValue() );
}


void uiVisIsoSurfaceThresholdDlg::mousePressed( CallBacker* cb )
{
    handleClick( cb, false );
}


void uiVisIsoSurfaceThresholdDlg::modeChangeCB( CallBacker* )
{
    const bool showseed = usemode_ && !modefld_->getBoolValue();
    ioobjselfld_->display( showseed );
    aboveisovaluefld_->display( showseed );
}


void uiVisIsoSurfaceThresholdDlg::doubleClick( CallBacker* cb )
{
    handleClick( cb, true );
}


uiFunctionDisplay& uiVisIsoSurfaceThresholdDlg::funcDisp()
{
    return *statsdisplay_->funcDisp();
}


uiAxisHandler& uiVisIsoSurfaceThresholdDlg::xAxis()
{
    return *statsdisplay_->funcDisp()->xAxis();
}


void uiVisIsoSurfaceThresholdDlg::handleClick( CallBacker* cb, bool isdouble )
{
    MouseEventHandler& eventhandler = funcDisp().scene().getMouseEventHandler();
    if ( eventhandler.isHandled() )
	return;

    const MouseEvent& event = eventhandler.event();
    if ( !event.leftButton() || event.rightButton() || event.middleButton() ||
	 event.ctrlStatus() || event.altStatus() || event.shiftStatus() )
	return;

    const uiPoint& pt = event.pos();

    eventhandler.setHandled( true );
    const float val = xAxis().getVal( pt.x );
    thresholdfld_->setValue( val );
    if ( isdouble )
	updateIsoDisplay( val );
}


void uiVisIsoSurfaceThresholdDlg::updateIsoDisplay( float nv )
{
    MouseCursorChanger changer( MouseCursor::Wait );
    vd_->setIsoValue( isosurfacedisplay_, nv );
}


void uiVisIsoSurfaceThresholdDlg::drawHistogram()
{
    LineStyle ls;
    ls.width_ = 2;
    uiLineItem* lineitem = new uiLineItem();
    if ( !mIsUdf(initialvalue_) )
    {
	ls.color_ = Color(0,150,0);
	const int val = xAxis().getPix(initialvalue_);
	lineitem = funcDisp().scene().addLine(val, 0, val, funcDisp().width());
	lineitem->setPenStyle( ls );
    }

    if ( !mIsUdf(thresholdfld_->getfValue() ) )
    {
	ls.color_ = Color(0,255,0,0); 
	const int val = xAxis().getPix(thresholdfld_->getfValue());
	lineitem = funcDisp().scene().addLine(val, 0, val, funcDisp().width());
    }

    if ( !mIsUdf(vd_->isoValue( isosurfacedisplay_ ) ) )
    {
	ls.color_ = Color(255,0,0,0);
	const int val = xAxis().getPix( vd_->isoValue( isosurfacedisplay_) );
	lineitem = funcDisp().scene().addLine(val, 0, val, funcDisp().width());
    }
}

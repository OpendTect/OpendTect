/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uidpscrossplotpropdlg.h"
#include "uidatapointsetcrossplot.h"

#include "mathexpression.h"

#include "linear.h"
#include "settings.h"

#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uicolortable.h"
#include "uiaxishandler.h"
#include "uilineedit.h"
#include "uimsg.h"

static const int cMinPtsForDensity = 20000;

struct uiDPSCPScalingTabAxFlds
{
    		uiDPSCPScalingTabAxFlds()
		    : doclipfld_(0), percclipfld_(0), rgfld_(0)	{}

    uiGenInput*	doclipfld_;
    uiGenInput*	percclipfld_;
    uiGenInput*	rgfld_;
};


class uiDPSCPScalingTab : public uiDlgGroup
{
public:

uiDPSCPScalingTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),"Scaling")
    , plotter_(p->plotter())
{
    const char* axnms[] = { "X", "Y", "Y2", 0 };
    uiLabeledComboBox* axlcb = new uiLabeledComboBox( this, axnms, "Axis" );
    axselfld_ = axlcb->box();
    const CallBack axselcb( mCB(this,uiDPSCPScalingTab,axSel) );
    axselfld_->selectionChanged.notify( axselcb );

    for ( int idx=0; idx<3; idx++ )
    {
	uiDPSCPScalingTabAxFlds* flds = new uiDPSCPScalingTabAxFlds;
	axflds_ += flds;
	uiAxisHandler* axhndlr = plotter_.axisHandler( idx );
	if ( !axhndlr ) continue;

	const uiAxisData::AutoScalePars& asp = plotter_.autoScalePars(idx);
	flds->doclipfld_ = new uiGenInput( this, "Use clipping",
				    BoolInpSpec(asp.doautoscale_) );
	flds->doclipfld_->valuechanged.notify(
			    mCB(this,uiDPSCPScalingTab,useClipSel) );
	flds->percclipfld_ = new uiGenInput( this, "Clipping percentage",
					FloatInpSpec(asp.clipratio_*100) );
	flds->doclipfld_->attach( alignedBelow, axlcb );
	flds->percclipfld_->attach( alignedBelow, flds->doclipfld_ );

	flds->rgfld_ = new uiGenInput( this, "Axis range/step",
		FloatInpIntervalSpec(axhndlr->range()) );
	flds->rgfld_->attach( alignedBelow, flds->doclipfld_ );
    }

    p->postFinalise().notify( axselcb );
}

void axSel( CallBacker* )
{
    const int axnr = axselfld_->currentItem();
    if ( axnr < 0 ) return;

    for ( int idx=0; idx<3; idx++ )
    {
	uiDPSCPScalingTabAxFlds& axflds = *axflds_[idx];
	if ( !axflds.doclipfld_ ) continue;
	
	uiAxisHandler* axhndlr = plotter_.axisHandler( idx );
	if ( !axhndlr ) continue;

	const uiAxisData::AutoScalePars& asp = plotter_.autoScalePars(idx);
	axflds.doclipfld_->display( idx == axnr );
	axflds.doclipfld_->setValue( asp.doautoscale_ );
	axflds.percclipfld_->display( idx == axnr );
	axflds.percclipfld_->setValue( asp.clipratio_*100 );
	axflds.rgfld_->display( idx == axnr );
	axflds.rgfld_->setValue( axhndlr->range() );
    }
    useClipSel( 0 );
}


void useClipSel( CallBacker* )
{
    const int axnr = axselfld_->currentItem();
    if ( axnr < 0 ) return;

    uiDPSCPScalingTabAxFlds& axflds = *axflds_[axnr];
    if ( !axflds.doclipfld_ ) return;

    const bool doclip = axflds.doclipfld_->getBoolValue();
    axflds.percclipfld_->display( doclip );
    axflds.rgfld_->display( !doclip );
}


bool acceptOK()
{
    for ( int idx=0; idx<3; idx++ )
    {
	uiAxisHandler* axh = plotter_.axisHandler( idx );
	if ( !axh ) continue;

	uiDPSCPScalingTabAxFlds& axflds = *axflds_[idx];
	uiAxisData::AutoScalePars& asp = plotter_.autoScalePars( idx );
	const bool doas = axflds.doclipfld_->getBoolValue();
	if ( !doas )
	    axh->setBounds( axflds.rgfld_->getFStepInterval() );
	else
	{
	    float cr = axflds.percclipfld_->getfValue() * 0.01f;
	    if ( cr < 0 || cr > 1 )
	    {
		uiMSG().error("Clipping percentage must be between 0 and 100");
		return false;
	    }
	    asp.clipratio_ = cr;
	}

	asp.doautoscale_ = plotter_.axisData(idx).needautoscale_ = doas;
    }

    return true;
}

    uiDataPointSetCrossPlotter&		plotter_;
    uiComboBox*				axselfld_;
    ObjectSet<uiDPSCPScalingTabAxFlds>	axflds_;

};


class uiDPSCPStatsTab : public uiDlgGroup
{
public:

uiDPSCPStatsTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),"Statistics")
    , plotter_(p->plotter())
{
    uiLabel* ylbl = new uiLabel( this, "Y =" );
    a0fld_ = new uiLineEdit( this, FloatInpSpec(0), "A0" );
    a0fld_->attach( rightOf, ylbl );
    uiLabel* pluslbl = new uiLabel( this, "+ " );
    pluslbl->attach( rightOf, a0fld_ );

    a1fld_ = new uiLineEdit( this, FloatInpSpec(1), "A1" );
    a1fld_->attach( rightOf, pluslbl );
    uiLabel* xlbl = new uiLabel( this, "* X" );
    xlbl->attach( rightOf, a1fld_ );

    d0fld_ = new uiLineEdit( this, FloatInpSpec(0), "D0" );
    d0fld_->attach( alignedBelow, a0fld_ );
    uiLabel* dlbl = new uiLabel( this, "Errors" );
    dlbl->attach( leftOf, d0fld_ );
    d1fld_ = new uiLineEdit( this, FloatInpSpec(0), "D1" );
    d1fld_->attach( alignedBelow, a1fld_ );

    ccfld_ = new uiLineEdit( this, FloatInpSpec(0), "CC" );
    ccfld_->attach( alignedBelow, d0fld_ );
    uiLabel* cclbl = new uiLabel( this, "Correlation coefficient" );
    cclbl->attach( leftOf, ccfld_ );
    ccdispbut_ = new uiCheckBox( this, "Put in plot" );
    ccdispbut_->attach( rightOf, ccfld_ );
    shwregrlnbut_ = new uiCheckBox( this, "Show regression line" );
    shwregrlnbut_->attach( alignedBelow, ccfld_ );

    a0fld_->setReadOnly( true );
    a1fld_->setReadOnly( true );
    d0fld_->setReadOnly( true );
    d1fld_->setReadOnly( true );
    ccfld_->setReadOnly( true );

    p->postFinalise().notify( mCB(this,uiDPSCPStatsTab,initFlds) );
}

void initFlds( CallBacker* )
{
    if ( !plotter_.axisHandler(0) || !plotter_.axisHandler(1) ) return;

    const LinStats2D& ls = plotter_.linStats();
    a0fld_->setValue( ls.lp.a0 );
    a1fld_->setValue( ls.lp.ax );
    d0fld_->setValue( ls.sd.a0 );
    d1fld_->setValue( ls.sd.ax );
    ccfld_->setValue( ls.corrcoeff );
    ccdispbut_->setChecked( plotter_.setup().showcc_ );
    shwregrlnbut_->setChecked( plotter_.setup().showregrline_ );
}

bool acceptOK()
{
    plotter_.setup().showcc( ccdispbut_->isChecked() );
    plotter_.setup().showregrline( shwregrlnbut_->isChecked() );
    return true;
}

    uiDataPointSetCrossPlotter&		plotter_;

    uiLineEdit*		a0fld_;
    uiLineEdit*		a1fld_;
    uiLineEdit*		d0fld_;
    uiLineEdit*		d1fld_;
    uiLineEdit*		ccfld_;
    uiCheckBox*		ccdispbut_;
    uiCheckBox*		shwregrlnbut_;

};


class uiDPSUserDefTab : public uiDlgGroup
{
public:

uiDPSUserDefTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),"User Defined")
    , plotter_(p->plotter())
    , dps_(p->plotter().dps())
    , hasy2_(plotter_.axisHandler(2))
    , yaxrg_((plotter_.axisData(1)).axis_->range())
    , y2axrg_((plotter_.axisData(hasy2_? 2:1)).axis_->range())
    , shwy1userdefpolyline_(0)
    , shwy2userdefpolyline_(0)
    , mathobj_(0)
    , mathobj1_(0)
    , exp1chgd_(false)
    , exp2chgd_(false)
    , selaxisfld_(0)
    , dragmode_(0)
{
    inpfld_ = new uiGenInput( this, "Equation Y1=" );
    inpfld_->setElemSzPol( uiObject::Wide );
    inpfld_->updateRequested.notify( mCB(this,uiDPSUserDefTab,parseExp) );
    inpfld_->valuechanging.notify( mCB(this,uiDPSUserDefTab,checkMathExpr) );

    rmsfld_ = new uiGenInput( this, "rms error" );
    rmsfld_->setElemSzPol( uiObject::Small );
    rmsfld_->attach( rightOf, inpfld_);

    if ( !mathexprstring_.isEmpty() )
    {
        inpfld_->setText( mathexprstring_ );
	parseExp(inpfld_);
    }    

    shwy1userdefpolyline_ = new uiCheckBox( this,"Show Y1 User Defined Curve" );
    shwy1userdefpolyline_->activated.notify(mCB(this,uiDPSUserDefTab,parseExp));
    shwy1userdefpolyline_->attach( alignedBelow, inpfld_ );
    
    if ( hasy2_ )
    {
	inpfld1_ = new uiGenInput( this, "Equation Y2=" );
	inpfld1_->setElemSzPol( uiObject::Wide );
	inpfld1_->updateRequested.notify( mCB(this,uiDPSUserDefTab,parseExp) );
	inpfld1_->valuechanging.notify( mCB(this,uiDPSUserDefTab,checkMathExpr) );
	inpfld1_->attach( alignedBelow, shwy1userdefpolyline_ );        

	rmsfld1_ = new uiGenInput( this, "rms error" );
	rmsfld1_->setElemSzPol( uiObject::Small );
	rmsfld1_->attach( rightOf, inpfld1_);
	
	
	if ( !mathexprstring1_.isEmpty() )
	{
	    inpfld1_->setText( mathexprstring1_ );
	    parseExp(inpfld1_);
	}

	shwy2userdefpolyline_ =
	    new uiCheckBox( this, "Show Y2 User Defined Curve" );
	shwy2userdefpolyline_->activated.notify(
		mCB(this,uiDPSUserDefTab,parseExp) );

	shwy2userdefpolyline_->attach( alignedBelow, inpfld1_ );
    }

    drawlinefld_ = new uiCheckBox( this, "Draw Line" );
    drawlinefld_->attach( alignedBelow, hasy2_ ? shwy2userdefpolyline_
	    : shwy1userdefpolyline_ );
    drawlinefld_->activated.notify( mCB(this,uiDPSUserDefTab,checkedCB) );
    
    if ( hasy2_ )
    {
	selaxisfld_ =
	    new uiGenInput( this, "", BoolInpSpec( true,"Draw Y1","Draw Y2" ) );
	selaxisfld_->attach( rightTo, drawlinefld_ );
	selaxisfld_->valuechanged.notify(
		mCB(this,uiDPSUserDefTab,drawAxisChanged) );
	selaxisfld_->display( false );
    }

    checkedCB( 0 );
    plotter_.lineDrawn.notify( mCB(this,uiDPSUserDefTab,setFlds) );
    p->postFinalise().notify( mCB(this,uiDPSUserDefTab,initFlds) );
    p->windowClosed.notify( mCB(this,uiDPSUserDefTab,setPolyLines) );
}


~uiDPSUserDefTab()
{
    plotter_.lineDrawn.remove( mCB(this,uiDPSUserDefTab,setFlds) );
}


void checkMathExpr( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,yinp,cb);
    const bool isy1 = (yinp && yinp==inpfld_);
    const BufferString& mathexpr = isy1 ? mathexprstring_ : mathexprstring1_;
    const BufferString& inptxt = isy1 ? inpfld_->text() : inpfld1_->text();
    bool& expchgd = isy1 ? exp1chgd_ : exp2chgd_;

    if ( mathexpr != inptxt )
    {
	expchgd = true;
	isy1 ? rmsfld_->setText(0) : rmsfld1_->setText(0);
    }
    else
    {
	expchgd = false;
	isy1 ? rmsfld_->setText(plotter_.y1rmserr_)
	    : rmsfld1_->setText(plotter_.y2rmserr_);
    }
}


void parseExp( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,yinp,cb);
    mDynamicCastGet(uiCheckBox*,ycb,cb);
    if ( !yinp && !ycb )
	return;

    const bool isy1 =
	(yinp && yinp==inpfld_) || (ycb && ycb==shwy1userdefpolyline_);
    BufferString& mathexpr = isy1 ? mathexprstring_ : mathexprstring1_;
    mathexpr = isy1 ? inpfld_->text() : inpfld1_->text();
    MathExpressionParser mep( mathexpr );
    MathExpression* mathobj = mep.parse();
    isy1 ? mathobj_ = mathobj : mathobj1_ = mathobj;
    if ( !mathobj )
    {
	if ( mep.errMsg() ) uiMSG().error( mep.errMsg() );
	return;
    }

    if ( mathobj->nrVariables() > 1 )
    {
	msg_ = "Expression of curve Y";
	msg_ += isy1 ? "1" : "2";
	msg_ += " contains more than one variable.";
	uiMSG().error( msg() );
	return;
    }
}


void drawAxisChanged( CallBacker* )
{
    plotter_.setUserDefDrawType( drawlinefld_->isChecked(),
	    !selaxisfld_->getBoolValue(),true );
}


void checkedCB( CallBacker* )
{
    if ( selaxisfld_ )
	selaxisfld_->display( drawlinefld_->isChecked() );
    
    plotter_.setUserDefDrawType( drawlinefld_->isChecked(),
	    selaxisfld_ && !selaxisfld_->getBoolValue(),true );

    MouseCursor cursor;
    if ( drawlinefld_->isChecked() )
    {
	dragmode_ = plotter_.dragMode();
	cursor.shape_ = MouseCursor::Cross;
	plotter_.setDragMode( uiGraphicsView::NoDrag );
    }
    else
	plotter_.setDragMode( (uiGraphicsView::ODDragMode)dragmode_ );
    
    plotter_.setCursor( cursor );
}


void initFlds( CallBacker* )
{
    if ( !plotter_.axisHandler(0) || !plotter_.axisHandler(1) ) return;
    
    inpfld_->setText( plotter_.userdefy1str_ );
    rmsfld_->setText( plotter_.y1rmserr_);
    
    if ( hasy2_ )
    {
	inpfld1_->setText( plotter_.userdefy2str_ );
	rmsfld1_->setText( plotter_.y2rmserr_);
	shwy2userdefpolyline_->setChecked( plotter_.setup().showy2userdefpolyline_ );
    }
    shwy1userdefpolyline_->setChecked( plotter_.setup().showy1userdefpolyline_ );
}


void setPolyLines( CallBacker* cb )
{
    TypeSet<uiWorldPoint> pos;
    pos += uiWorldPoint(0,0);
    mDynamicCastGet(uiDialog*,dlg,cb);
    if ( dlg && dlg->uiResult() == 1 )
	return;
    else if ( dlg->uiResult() == 0 )
    {
	shwy1userdefpolyline_->setChecked( false );
	if ( shwy2userdefpolyline_ )
	    shwy2userdefpolyline_->setChecked( false );
	plotter_.setUserDefDrawType( false, false );
	plotter_.setUserDefPolyLine( pos, false );
	plotter_.setUserDefDrawType( false, true );
	plotter_.setUserDefPolyLine( pos, true );
    }

    drawlinefld_->setChecked( false );
    if ( !shwy1userdefpolyline_->isChecked() )
	plotter_.setUserDefPolyLine( pos, false );
    if ( shwy2userdefpolyline_ && !shwy2userdefpolyline_->isChecked() ) 
	plotter_.setUserDefPolyLine( pos, true );
    plotter_.setDragMode( (uiGraphicsView::ODDragMode)dragmode_ );
}


void drawPolyLines()
{
    uiDataPointSetCrossPlotter::AxisData& yax = plotter_.axisData(1);
    const bool& yrgchgd = ( yrgchgd_ = !( yax.axis_->range() == yaxrg_ ) );
    const bool shwy1 = ( shwy1userdefpolyline_->isChecked() &&
	    !mathexprstring_.isEmpty() && !(mathobj_->nrVariables()>1) );

    if ( shwy1 && ( exp1chgd_ || yrgchgd ) )
    {
    	yax.autoscalepars_.doautoscale_ = yax.needautoscale_ = !yrgchgd;
	computePts( false );
	exp1chgd_ = false;
    }
    else if ( !exp1chgd_ )
    {
	yax.needautoscale_ = false;
    }
    plotter_.setUserDefDrawType( shwy1,false );

    if ( hasy2_ )
    {
	uiDataPointSetCrossPlotter::AxisData& y2ax = plotter_.axisData(2);
	const bool& y2rgchgd = ( y2rgchgd_= !(y2ax.axis_->range()==y2axrg_) );
	const bool shwy2 = ( shwy2userdefpolyline_->isChecked() &&
		!mathexprstring1_.isEmpty() && !(mathobj1_->nrVariables()>1) );

    	if ( shwy2 && ( exp2chgd_ || y2rgchgd ) )
    	{
	    y2ax.autoscalepars_.doautoscale_ = y2ax.needautoscale_ = !y2rgchgd;
	    computePts( true );
	    exp2chgd_ = false;
	}
	else if ( !exp2chgd_ )
	{   
	    y2ax.needautoscale_ = false;
	}
	plotter_.setUserDefDrawType( shwy2,true );
    }
}


void computePts( bool isy2 )
{
    TypeSet<uiWorldPoint> pts;
    TypeSet<uiWorldPoint> validpts; 
    uiDataPointSetCrossPlotter::AxisData& horz = plotter_.axisData(0);
    uiDataPointSetCrossPlotter::AxisData& vert = plotter_.axisData(isy2 ? 2:1);

    const BinIDValueSet& bvs = dps_.bivSet();
    BinIDValueSet::Pos pos;
    float rmserr = 0;
    int count = 0;
    while ( bvs.next(pos,false) )
    {
	BinID curbid;
	TypeSet<float> vals;
	bvs.get( pos, curbid, vals );
	DataPointSet::RowID rid = dps_.getRowID( pos );

	const float xval = plotter_.getVal( horz.colid_, rid );
	const float yval = plotter_.getVal( vert.colid_, rid );
		
	if ( mIsUdf(xval) || mIsUdf(yval) )
	    continue;

	if ( isy2 )
	    mathobj1_->setVariableValue( 0, xval );
	else
	    mathobj_->setVariableValue( 0, xval );

	float expyval = isy2 ? mathobj1_->getValue() : mathobj_->getValue();

	if ( mIsUdf(expyval) ) continue;

	rmserr += ( expyval - yval )*( expyval - yval );	
	count += 1;
    }

    if ( count != 0 )
    {
	rmserr = sqrt(rmserr/(float)count);
    	isy2 ? rmsfld1_->setValue( rmserr ) : rmsfld_->setValue( rmserr );
    	( isy2 ? plotter_.y2rmserr_ : plotter_.y1rmserr_ ) = rmserr;
    }
    else
    {
	isy2 ? rmsfld1_->setText(0) : rmsfld_->setText(0);     	
	isy2 ? plotter_.y2rmserr_.setEmpty() : plotter_.y1rmserr_.setEmpty();
    }

    StepInterval<float> curvyvalrg( mUdf(float), -mUdf(float),
	    vert.axis_->range().step );
    Interval<float> xrge = horz.rg_;
    const float step = fabs( ( xrge.stop - xrge.start )/999.0f );

    for ( int idx = 0; idx < 1000; idx++ )
    {
	float curvxval = xrge.start + ((float)idx)*step;

	if ( isy2 )
	    mathobj1_->setVariableValue( 0, curvxval );
	else
	    mathobj_->setVariableValue( 0, curvxval );
	
	float curvyval = isy2 ? mathobj1_->getValue() : mathobj_->getValue();

	if ( mIsUdf(curvxval) || mIsUdf(curvyval) )
	    continue;

	if ( vert.axis_->range().includes(curvyval,false) )
	    validpts += uiWorldPoint( curvxval, curvyval );
	
	curvyvalrg.include( curvyval, false );
	pts += uiWorldPoint( curvxval, curvyval );
    }
    
    if ( pts.size()==0 )
    {
	msg_ = "Sorry! Y";
	msg_ += isy2 ? 2 : 1;
        msg_ += " cannot be plotted.";
	uiMSG().error( msg() );
	return;
    }

    const bool& vertrgchgd = isy2 ? y2rgchgd_ : yrgchgd_;

    if ( !vert.axis_->range().includes(curvyvalrg) )
    {
	msg_ = "Curve for Y";
	msg_ += isy2 ? 2 : 1;
	msg_ += " goes beyond the default range. ";
	msg_ += "Do you want to rescale to see the complete curve?";

	if ( !vertrgchgd && uiMSG().askGoOn(msg_) )
	{
	    curvyvalrg.include( vert.axis_->range(), false );
	    curvyvalrg.step = (curvyvalrg.stop - curvyvalrg.start)/4.0f;
	    vert.autoscalepars_.doautoscale_ = vert.needautoscale_ = false;
	    vert.axis_->setBounds( curvyvalrg );
	    plotter_.setUserDefPolyLine( pts,isy2 );
	}
	else
	    plotter_.setUserDefPolyLine( validpts,isy2 );
    }
    else
	plotter_.setUserDefPolyLine( pts,isy2 );

    ( isy2 ? y2axrg_ : yaxrg_ ) = vert.axis_->range();
}


void setFlds( CallBacker* )
{
    if ( drawlinefld_->isChecked() )
    {
	if ( selaxisfld_ && !selaxisfld_->getBoolValue() )
	{
	    inpfld1_->setText( plotter_.userdefy2str_ );
	}
	else
	{
	    inpfld_->setText( plotter_.userdefy1str_ );
	}
    }
}


bool acceptOK()
{
    plotter_.userdefy1str_ = inpfld_->text();
    drawlinefld_->setChecked( false );

    if ( plotter_.userdefy1str_.isEmpty() )
    {
	shwy1userdefpolyline_->setChecked( false );
	plotter_.y1rmserr_.setEmpty();
    }

    if ( hasy2_ )
    {
	plotter_.userdefy2str_ = inpfld1_->text();
	if ( plotter_.userdefy2str_.isEmpty() )
	{
	    shwy2userdefpolyline_->setChecked( false );
	    plotter_.y2rmserr_.setEmpty();
	}

	plotter_.setup().showy2userdefpolyline_
	    = shwy2userdefpolyline_->isChecked();
    }
    plotter_.setup().showy1userdefpolyline_=shwy1userdefpolyline_->isChecked();


    if ( shwy1userdefpolyline_->isChecked() )
	parseExp( shwy1userdefpolyline_ );
    
    if ( hasy2_ && shwy2userdefpolyline_->isChecked() )
	parseExp( shwy2userdefpolyline_ );

    drawPolyLines();
    return true;
}

    uiDataPointSetCrossPlotter&		plotter_;
    const DataPointSet&                 dps_;

    bool		 		hasy2_;
    bool				exp1chgd_;
    bool				exp2chgd_;
    bool				yrgchgd_;
    bool				y2rgchgd_;
    int  				dragmode_;
    uiGenInput*                         inpfld_;
    uiGenInput*                         inpfld1_;
    uiGenInput*				rmsfld_;
    uiGenInput*				rmsfld1_;
    uiGenInput*				selaxisfld_;
    uiCheckBox*				shwy1userdefpolyline_;
    uiCheckBox*				shwy2userdefpolyline_;
    uiCheckBox*				drawlinefld_;
    BufferString			mathexprstring_;
    BufferString       			mathexprstring1_;
    MathExpression*			mathobj_;
    MathExpression*			mathobj1_;
    StepInterval<float>			yaxrg_;
    StepInterval<float>			y2axrg_;
    const char*	                 	msg() const  { return msg_.str(); }
    mutable BufferString        	msg_;
};


class uiDPSCPDisplayPropTab : public uiDlgGroup
{
public:

uiDPSCPDisplayPropTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),"Display Properties")
    , plotter_(p->plotter())
{
    const MarkerStyle2D& mstyle = plotter_.setup().markerstyle_;
    sizefld_ = new uiGenInput( this, "Marker size", IntInpSpec(mstyle.size_));
    BufferStringSet shapenms;
    shapenms.add( "Square" );
    shapenms.add( "Circle" );
    shapenms.add( "Cross" );
    shapenms.add( "Plus" );
    shapenms.add( "Target" );
    shapenms.add( "HLine" );
    shapenms.add( "VLine" );
    shapenms.add( "Plane" );
    shapenms.add( "Triangle" );
    shapenms.add( "Arrow" );

    uiLabeledComboBox* llb =
	new uiLabeledComboBox( this, shapenms, "Marker shape" );
    shapefld_ = llb->box();
    llb->attach( alignedBelow, sizefld_ );
    shapefld_->setCurrentItem( (int)(mstyle.type_-1) );

    Color yaxiscol = plotter_.axisHandler(1)->setup().style_.color_;
    ycolinpfld_ = new uiColorInput( this, uiColorInput::Setup(yaxiscol)
	    					.lbltxt("Y Axis Color") );
    ycolinpfld_->attach( alignedBelow, llb );
    
    if ( plotter_.isY2Shown() )
    {
	Color y2axiscol = plotter_.axisHandler(2)->setup().style_.color_;
	y2colinpfld_ = new uiColorInput( this, uiColorInput::Setup(y2axiscol)
						.lbltxt("Y2 Axis Color") );
	y2colinpfld_->attach( alignedBelow, ycolinpfld_ );
    }
}

bool acceptOK()
{
    if ( sizefld_->getIntValue() <= 0 )
    {
	uiMSG().error( "Cannot put negative size for size." );
	return false;
    }	

    MarkerStyle2D& mstyle = plotter_.setup().markerstyle_;
    mstyle.size_ = sizefld_->getIntValue();
    mstyle.type_ = (MarkerStyle2D::Type)(shapefld_->currentItem()+1);
    plotter_.axisHandler(1)->setup().style_.color_ = ycolinpfld_->color();
    if ( plotter_.isY2Shown() ) 
	plotter_.axisHandler(2)->setup().style_.color_ = y2colinpfld_->color();
    return true;
}


    uiDataPointSetCrossPlotter& plotter_;
    uiGenInput*			sizefld_;
    uiComboBox*			shapefld_;
    uiColorInput*		ycolinpfld_;
    uiColorInput*		y2colinpfld_;
};


class uiDPSDensPlotSetTab : public uiDlgGroup
{
public:

uiDPSDensPlotSetTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),"Density Plot")
    , plotter_(p->plotter())
{
    Settings& setts = Settings::common();
    if ( !setts.get(sKeyMinDPPts(),minptsfordensity_) )
	minptsfordensity_ = cMinPtsForDensity;
    BufferString msg( "Current Number of Points (including undefined values) ");
    msg += plotter_.totalNrItems();
    uiLabel* lbl = new uiLabel( this, msg );
    
    const int cellsize = plotter_.cellSize();
    cellsize_ = cellsize;
    minptinpfld_ =
	new uiGenInput( this, "Threshold minimum points for Density Plot",
			IntInpSpec(minptsfordensity_) );
    minptinpfld_->attach( rightAlignedBelow, lbl );
    
    cellsizefld_ = new uiGenInput( this, "Cell Size", IntInpSpec(cellsize) );
    cellsizefld_->attach( alignedBelow, minptinpfld_ );
    cellsizefld_->valuechanged.notify(
	    mCB(this,uiDPSDensPlotSetTab,cellSzChanged) );
    
    int width = 0;
    int height = 0;
    if ( plotter_.axisHandler(0) )
	width = plotter_.axisHandler(0)->pixRange().width();
    if ( plotter_.axisHandler(1) )
	height = plotter_.axisHandler(1)->pixRange().width();
    wcellszfld_ = new uiGenInput( this, "Nr of Cells across Width",
				  IntInpSpec(width/cellsize) );
    wcellszfld_->attach( alignedBelow, cellsizefld_ );
    wcellszfld_->valuechanged.notify(
	    mCB(this,uiDPSDensPlotSetTab,wCellNrChanged) );
    hcellszfld_ = new uiGenInput( this, "Nr of Cells across Height",
				  IntInpSpec(height/cellsize) );
    hcellszfld_->attach( alignedBelow, wcellszfld_ );
    hcellszfld_->valuechanged.notify(
	    mCB(this,uiDPSDensPlotSetTab,hCellNrChanged) );
}

void cellSzChanged( CallBacker* )
{
    int cellsz = cellsizefld_->getIntValue();
    if ( mIsUdf(cellsz) || cellsz <=0 )
    {
	cellsizefld_->setValue( cellsize_ );
	cellsz = cellsizefld_->getIntValue();
    }

    wcellszfld_->setValue( mNINT32( (float)plotter_.arrArea().width()/cellsz) );
    hcellszfld_->setValue( mNINT32( (float) plotter_.arrArea().height()/cellsz) );
}

void wCellNrChanged( CallBacker* )
{
    const int cellsz = cellsizefld_->getIntValue();
    const float aspectratio = (float)(plotter_.arrArea().width()/cellsz)/
			      (float)(plotter_.arrArea().height()/cellsz);
    hcellszfld_->setValue( wcellszfld_->getIntValue()/aspectratio );
    cellsizefld_->setValue(
	    mNINT32((float) plotter_.arrArea().width()/wcellszfld_->getIntValue()) );
    
    if ( mIsUdf(cellsz) || cellsz <=0 )
	cellsizefld_->setValue( cellsize_ );
    cellSzChanged( 0 );
}

void hCellNrChanged( CallBacker* )
{
    const int cellsz = cellsizefld_->getIntValue();
    const float aspectratio = (float)(plotter_.arrArea().width()/cellsz)/
			      (float)(plotter_.arrArea().height()/cellsz);
    wcellszfld_->setValue( hcellszfld_->getIntValue()*aspectratio );
    cellsizefld_->setValue(
	    mNINT32((float) plotter_.arrArea().height()/hcellszfld_->getIntValue()) );
    
    if ( mIsUdf(cellsz) || cellsz <=0 )
	cellsizefld_->setValue( cellsize_ );
    cellSzChanged( 0 );
}

bool acceptOK()
{
    minptsfordensity_ = minptinpfld_->getIntValue();
    if ( cellsizefld_->getIntValue() <= 0 )
    {
	uiMSG().error( "Cannot have a cellsize less than 1" );
	return false;
    }

    if ( plotter_.cellSize() != cellsizefld_->getIntValue() )
	plotter_.setCellSize( cellsizefld_->getIntValue() );
    Settings& setts = Settings::common();
    setts.set( sKeyMinDPPts(), minptsfordensity_ );
    return setts.write();
}

    uiDataPointSetCrossPlotter& plotter_;
    uiGenInput*			minptinpfld_;
    uiGenInput*			cellsizefld_;
    uiGenInput*			wcellszfld_;
    uiGenInput*			hcellszfld_;
    int				minptsfordensity_;
    int				cellsize_;

    static const char*		sKeyMinDPPts()
    				{ return "Minimum pts for Density Plot"; }
};


uiDataPointSetCrossPlotterPropDlg::uiDataPointSetCrossPlotterPropDlg(
		uiDataPointSetCrossPlotter* p )
	: uiTabStackDlg( p->parent(), uiDialog::Setup("Settings",0,"111.0.2")
						.modal(false) )
	, plotter_(*p)
    	, bdroptab_(0)
{
    setDeleteOnClose( false );
    scaletab_ = new uiDPSCPScalingTab( this );
    addGroup( scaletab_ );
    statstab_ = new uiDPSCPStatsTab( this );
    addGroup( statstab_ );
    userdeftab_ = new uiDPSUserDefTab( this );
    addGroup( userdeftab_ );
    dispproptab_ = new uiDPSCPDisplayPropTab( this );
    addGroup( dispproptab_ );
    densplottab_ = new uiDPSDensPlotSetTab( this );
    addGroup( densplottab_ );

    uiPushButton* applybut = new uiPushButton( this, "&Apply",
	    mCB(this,uiDataPointSetCrossPlotterPropDlg,doApply), true );
    applybut->attach( centeredBelow, tabObject() );
}


void uiDataPointSetCrossPlotterPropDlg::doApply( CallBacker* cb )
{ acceptOK( cb ); }

bool uiDataPointSetCrossPlotterPropDlg::acceptOK( CallBacker* )
{
    if ( scaletab_ ) scaletab_->acceptOK();
    if ( statstab_ ) statstab_->acceptOK();
    if ( userdeftab_ ) userdeftab_->acceptOK();
    if ( dispproptab_ ) dispproptab_->acceptOK();
    if ( densplottab_ ) densplottab_->acceptOK();

    plotter_.dataChanged();
    return true;
}

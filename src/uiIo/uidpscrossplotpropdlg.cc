/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2008
________________________________________________________________________

-*/

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
#include "uilineedit.h"
#include "uimsg.h"
#include "od_helpids.h"

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
{ mODTextTranslationClass(uiDPSCPScalingTab);
public:

uiDPSCPScalingTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),tr("Scaling"))
    , plotter_(p->plotter())
{
    const char* axnms[] = { "X", "Y", "Y2", 0 };
    uiLabeledComboBox* axlcb = new uiLabeledComboBox( this, axnms,
							  uiStrings::sAxis() );
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
	flds->doclipfld_ = new uiGenInput( this, tr("Use clipping"),
				BoolInpSpec(asp.doautoscale_) );
	flds->doclipfld_->valuechanged.notify(
				mCB(this,uiDPSCPScalingTab,useClipSel) );
	flds->percclipfld_ = new uiGenInput( this, tr("Clipping percentage"),
				FloatInpSpec(asp.clipratio_*100) );
	flds->doclipfld_->attach( alignedBelow, axlcb );
	flds->percclipfld_->attach( alignedBelow, flds->doclipfld_ );

	flds->rgfld_ = new uiGenInput( this, uiStrings::phrJoinStrings(
				uiStrings::sAxis(),tr("range/step")),
				FloatInpIntervalSpec(axhndlr->range()) );
	flds->rgfld_->attach( alignedBelow, flds->doclipfld_ );
    }

    plotter_.dataChgd.notify( mCB(this,uiDPSCPScalingTab,updateFlds) );
    p->postFinalise().notify( axselcb );
}


~uiDPSCPScalingTab()
{
    plotter_.dataChgd.remove( mCB(this,uiDPSCPScalingTab,updateFlds) );
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

	axflds.doclipfld_->display( idx == axnr );
	axflds.percclipfld_->display( idx == axnr );
	axflds.rgfld_->display( idx == axnr );
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


void updateFlds( CallBacker* )
{
    for ( int idx=0; idx<3; idx++ )
    {
	uiAxisHandler* axh = plotter_.axisHandler( idx );
	if ( !axh ) continue;

	uiDPSCPScalingTabAxFlds& axflds = *axflds_[idx];
	uiAxisData::AutoScalePars& asp = plotter_.autoScalePars( idx );

	axflds.doclipfld_->setValue(asp.doautoscale_);
	axflds.rgfld_->setValue(axh->range());
	useClipSel( 0 );
    }
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
	    axh->setRange( axflds.rgfld_->getFStepInterval() );
	else
	{
	    float cr = axflds.percclipfld_->getFValue() * 0.01f;
	    if ( cr < 0 || cr > 1 )
	    {
		uiMSG().error(tr("Clipping percentage must "
				 "be between 0 and 100"));
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
{ mODTextTranslationClass(uiDPSCPStatsTab)
public:

uiDPSCPStatsTab( uiDataPointSetCrossPlotterPropDlg* p, bool y1 )
    : uiDlgGroup(p->tabParent(),
	toUiString("%1 %2").arg(y1 ? "Y1" : "Y2").arg(uiStrings::sStatistics()))
    , plotter_(p->plotter())
    , y1_(y1)
{
    uiLabel* ylbl = new uiLabel( this, toUiString("%1 =").arg(uiStrings::sY()));
    a0fld_ = new uiLineEdit( this, FloatInpSpec(0), "A0" );
    a0fld_->attach( rightOf, ylbl );
    uiLabel* pluslbl = new uiLabel( this, toUiString("+ ") );
    pluslbl->attach( rightOf, a0fld_ );

    a1fld_ = new uiLineEdit( this, FloatInpSpec(1), "A1" );
    a1fld_->attach( rightOf, pluslbl );
    uiLabel* xlbl = new uiLabel(this, toUiString("* %1").arg(uiStrings::sX()));
    xlbl->attach( rightOf, a1fld_ );

    d0fld_ = new uiLineEdit( this, FloatInpSpec(0), "D0" );
    d0fld_->attach( alignedBelow, a0fld_ );
    uiLabel* dlbl = new uiLabel( this, uiStrings::sErrors() );
    dlbl->attach( leftOf, d0fld_ );
    d1fld_ = new uiLineEdit( this, FloatInpSpec(0), "D1" );
    d1fld_->attach( alignedBelow, a1fld_ );

    ccfld_ = new uiLineEdit( this, FloatInpSpec(0), "CC" );
    ccfld_->attach( alignedBelow, d0fld_ );
    uiLabel* cclbl = new uiLabel( this, uiStrings::sCorrelCoeff());
    cclbl->attach( leftOf, ccfld_ );
    ccdispbut_ = new uiCheckBox( this, tr("Put in plot") );
    ccdispbut_->attach( rightOf, ccfld_ );
    shwregrlnbut_ = new uiCheckBox( this, tr("Show regression line") );
    shwregrlnbut_->attach( alignedBelow, ccfld_ );

    a0fld_->setReadOnly( true );
    a1fld_->setReadOnly( true );
    d0fld_->setReadOnly( true );
    d1fld_->setReadOnly( true );
    ccfld_->setReadOnly( true );
    a0fld_->setNrDecimals( 4 );
    a1fld_->setNrDecimals( 4 );
    d0fld_->setNrDecimals( 4 );
    d1fld_->setNrDecimals( 4 );
    ccfld_->setNrDecimals( 4 );

    mAttachCB( p->postFinalise(), uiDPSCPStatsTab::finalizeCB );
    mAttachCB( plotter_.dataChgd, uiDPSCPStatsTab::dataChangedCB );
}

~uiDPSCPStatsTab()
{
    detachAllNotifiers();
}

void finalizeCB( CallBacker* )
{
    updateFields();
}


void dataChangedCB( CallBacker* )
{
    updateFields();
}


void updateFields()
{
    const LinStats2D& ls = plotter_.linStats( y1_ );
    a0fld_->setValue( ls.lp.a0 );
    a1fld_->setValue( ls.lp.ax );
    d0fld_->setValue( ls.sd.a0 );
    d1fld_->setValue( ls.sd.ax );
    ccfld_->setValue( ls.corrcoeff );

    ccdispbut_->setChecked( plotter_.correlationCoeffShown(y1_) );
    shwregrlnbut_->setChecked( plotter_.regressionLineShown(y1_) );
}


bool acceptOK()
{
    plotter_.showRegressionLine( y1_, shwregrlnbut_->isChecked() );
    plotter_.showCorrelationCoeff( y1_, ccdispbut_->isChecked() );
    return true;
}

    uiDataPointSetCrossPlotter&		plotter_;

    bool		y1_;
    uiLineEdit*		a0fld_;
    uiLineEdit*		a1fld_;
    uiLineEdit*		d0fld_;
    uiLineEdit*		d1fld_;
    uiLineEdit*		ccfld_;
    uiCheckBox*		ccdispbut_;
    uiCheckBox*		shwregrlnbut_;

};


class uiDPSUserDefTab : public uiDlgGroup
{ mODTextTranslationClass(uiDPSUserDefTab);
public:

uiDPSUserDefTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),tr("User Defined"))
    , plotter_(p->plotter())
    , dps_(p->plotter().dps())
    , hasy2_(plotter_.axisHandler(2))
    , mathexprstring_(plotter_.userdefy1str_)
    , mathexprstring1_(plotter_.userdefy2str_)
    , shwy1userdefpolyline_(0)
    , shwy2userdefpolyline_(0)
    , mathobj_(0)
    , mathobj1_(0)
    , exp1plotted_(true)
    , exp2plotted_(true)
    , line1drawn_(false)
    , line2drawn_(false)
    , err1bfrplot_(false)
    , err2bfrplot_(false)
    , selaxisfld_(0)
    , dragmode_(uiGraphicsView::NoDrag)
{
    inpfld_ = new uiGenInput( this, tr("Equation Y1=") );
    inpfld_->setElemSzPol( uiObject::Wide );
    inpfld_->updateRequested.notify( mCB(this,uiDPSUserDefTab,parseExpCB) );
    inpfld_->valuechanging.notify( mCB(this,uiDPSUserDefTab,checkMathExpr) );

    rmsfld_ = new uiGenInput( this, mJoinUiStrs(sRMS(), sErrors()) );
    rmsfld_->setElemSzPol( uiObject::Small );
    rmsfld_->attach( rightOf, inpfld_);
    rmsfld_->setReadOnly( true );

    shwy1userdefpolyline_ = new uiCheckBox( this,
					    tr("Show Y1 User Defined Curve") );
    shwy1userdefpolyline_->activated.notify(
					mCB(this,uiDPSUserDefTab,parseExpCB));
    shwy1userdefpolyline_->attach( alignedBelow, inpfld_ );

    if ( hasy2_ )
    {
	inpfld1_ = new uiGenInput( this, tr("Equation Y2=") );
	inpfld1_->setElemSzPol( uiObject::Wide );
	inpfld1_->updateRequested.notify( mCB(this,uiDPSUserDefTab,parseExpCB));
	inpfld1_->valuechanging.notify(mCB(this,uiDPSUserDefTab,checkMathExpr));
	inpfld1_->attach( alignedBelow, shwy1userdefpolyline_ );

	rmsfld1_ = new uiGenInput( this, mJoinUiStrs(sRMS(), sErrors()) );
	rmsfld1_->setElemSzPol( uiObject::Small );
	rmsfld1_->attach( rightOf, inpfld1_);
	rmsfld1_->setReadOnly( true );

	shwy2userdefpolyline_ =
	    new uiCheckBox( this, tr("Show Y2 User Defined Curve") );
	shwy2userdefpolyline_->activated.notify(
		mCB(this,uiDPSUserDefTab,parseExpCB) );

	shwy2userdefpolyline_->attach( alignedBelow, inpfld1_ );
    }

    drawlinefld_ = new uiCheckBox( this, mJoinUiStrs(sDraw(),sLine()) );
    drawlinefld_->attach( alignedBelow, hasy2_ ? shwy2userdefpolyline_
	    : shwy1userdefpolyline_ );
    drawlinefld_->activated.notify( mCB(this,uiDPSUserDefTab,checkedCB) );

    if ( hasy2_ )
    {
	selaxisfld_ =
	    new uiGenInput( this, uiString::emptyString(),
                            BoolInpSpec( true,uiStrings::phrJoinStrings(
			    uiStrings::sDraw(),tr("Y1")),
			    mJoinUiStrs(sDraw(), sY2())) );
	selaxisfld_->attach( rightTo, drawlinefld_ );
	selaxisfld_->valuechanged.notify(
		mCB(this,uiDPSUserDefTab,drawAxisChanged) );
	selaxisfld_->display( false );
    }

    checkedCB( 0 );
    plotter_.lineDrawn.notify( mCB(this,uiDPSUserDefTab,setFlds) );
    plotter_.mouseReleased.notify( mCB(this,uiDPSUserDefTab,getRmsErrorCB) );
    p->postFinalise().notify( mCB(this,uiDPSUserDefTab,initFlds) );
    p->windowClosed.notify( mCB(this,uiDPSUserDefTab,setPolyLines) );
}


~uiDPSUserDefTab()
{
    plotter_.lineDrawn.remove( mCB(this,uiDPSUserDefTab,setFlds) );
    plotter_.mouseReleased.remove( mCB(this,uiDPSUserDefTab,getRmsErrorCB) );
    delete mathobj_; delete mathobj1_;
}


void checkMathExpr( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,yinp,cb);
    const bool isy1 = (yinp && yinp==inpfld_);
    const BufferString& mathexpr = isy1 ? mathexprstring_ : mathexprstring1_;
    const BufferString& inptxt = isy1 ? inpfld_->text() : inpfld1_->text();
    const bool& errbfrplot = isy1 ? err1bfrplot_ : err2bfrplot_;
    bool& expplotted = isy1 ? exp1plotted_ : exp2plotted_;
    bool& linedrawn = isy1 ? line1drawn_ : line2drawn_;

    if ( mathexpr != inptxt )
    {
	expplotted = linedrawn = false;
	isy1 ? rmsfld_->setText(0) : rmsfld1_->setText(0);
    }
    else
    {
	if ( !errbfrplot ) expplotted = true;
	isy1 ? rmsfld_->setText(plotter_.y1rmserr_)
	    : rmsfld1_->setText(plotter_.y2rmserr_);
    }
}


void parseExpCB( CallBacker* cb )
{
    parseExp( cb );
}


bool parseExp( CallBacker* cb )
{
    mDynamicCastGet(uiGenInput*,yinp,cb);
    mDynamicCastGet(uiCheckBox*,ycb,cb);
    if ( !yinp && !ycb ) return false;
    if ( ycb && !ycb->isChecked() ) return false;

    const bool isy1 =
	(yinp && yinp==inpfld_) || (ycb && ycb==shwy1userdefpolyline_);
    const BufferString& rmsstr = isy1 ? rmsfld_->text() : rmsfld1_->text();
    if ( !rmsstr.isEmpty() ) return false;

    BufferString& mathexpr = isy1 ? mathexprstring_ : mathexprstring1_;
    mathexpr = isy1 ? inpfld_->text() : inpfld1_->text();
    if ( mathexpr.isEmpty() ) return false;
    isy1 ? plotter_.y1rmserr_.setEmpty() : plotter_.y2rmserr_.setEmpty();;
    Math::ExpressionParser mep( mathexpr );
    Math::Expression*& mathobj = isy1 ? mathobj_ : mathobj1_;
    delete mathobj; mathobj = mep.parse();
    uiCheckBox* chkbox = isy1 ? shwy1userdefpolyline_ : shwy2userdefpolyline_;

    if ( !mathobj )
    {
	if ( mep.errMsg() )
	{
	    uiMSG().error( mToUiStringTodo(mep.errMsg()) );
	    chkbox->setChecked( false );
	}
	else if ( !mathexpr.isEmpty() )
	{
	    msg_ = tr("Expression for Y%1 is invalid.")
		 .arg(isy1 ? "1" : "2");
	    uiMSG().error( msg() );
	    chkbox->setChecked( false );
	}
	return false;
    }

    if ( mathobj->nrVariables() > 1 )
    {
	msg_ = tr("Expression for Y%1 contains more than one variable.")
	     .arg(isy1 ? "1" : "2");
	uiMSG().error( msg() );
	chkbox->setChecked( false );
	return false;
    }

    return true;
}


void drawAxisChanged( CallBacker* )
{
    plotter_.setUserDefDrawType( drawlinefld_->isChecked(),
	    !selaxisfld_->getBoolValue(), drawlinefld_->isChecked() );
}


void checkedCB( CallBacker* )
{
    if ( selaxisfld_ )
	selaxisfld_->display( drawlinefld_->isChecked() );

    plotter_.setUserDefDrawType( drawlinefld_->isChecked(),
	    selaxisfld_ && !selaxisfld_->getBoolValue(),
	    drawlinefld_->isChecked() );

    MouseCursor cursor;
    if ( drawlinefld_->isChecked() )
    {
	dragmode_ = plotter_.dragMode();
	cursor.shape_ = MouseCursor::Cross;
	plotter_.setDragMode( uiGraphicsView::NoDrag );
    }
    else
	plotter_.setDragMode( dragmode_ );

    plotter_.setCursor( cursor );
}


void initFlds( CallBacker* )
{
    inpfld_->setText( plotter_.userdefy1str_ );
    rmsfld_->setText( plotter_.y1rmserr_ );

    if ( hasy2_ )
    {
	inpfld1_->setText( plotter_.userdefy2str_ );
	rmsfld1_->setText( plotter_.y2rmserr_ );
	shwy2userdefpolyline_->setChecked(
	    plotter_.setup().showy2userdefpolyline_ );
    }
    shwy1userdefpolyline_->setChecked(plotter_.setup().showy1userdefpolyline_);
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
	if ( line1drawn_ || err1bfrplot_ )
	{
	    plotter_.setup().showy1userdefpolyline_ = false;
	    plotter_.setUserDefPolyLine( pos, false );
	    plotter_.drawUserDefPolyLine( true );
	}
	if ( shwy2userdefpolyline_ && ( line2drawn_ || err2bfrplot_ ) )
	{
	    plotter_.setup().showy2userdefpolyline_ = false;
	    plotter_.setUserDefPolyLine( pos, true );
	    plotter_.drawUserDefPolyLine( false );
	}
    }

    drawlinefld_->setChecked( false );
    if ( !shwy1userdefpolyline_->isChecked() )
	plotter_.setUserDefPolyLine( pos, false );
    if ( shwy2userdefpolyline_ && !shwy2userdefpolyline_->isChecked() )
	plotter_.setUserDefPolyLine( pos, true );
    plotter_.setDragMode( dragmode_ );
}


void drawPolyLines()
{
    uiDataPointSetCrossPlotter::AxisData& yax = plotter_.axisData(1);
    const bool shwy1 = shwy1userdefpolyline_->isChecked();

    if ( shwy1 && !exp1plotted_ )
    {
	yax.autoscalepars_.doautoscale_ = yax.needautoscale_ = true;
	computePts( false ); exp1plotted_ = true;
	line1drawn_ = err1bfrplot_ = false;
    }
    else if ( !exp1plotted_ && mathexprstring_.isEmpty() )
    {
	yax.autoscalepars_.doautoscale_ = yax.needautoscale_ = true;
	exp1plotted_ = true;
    }
    plotter_.setUserDefDrawType( shwy1,false );

    if ( hasy2_ )
    {
	uiDataPointSetCrossPlotter::AxisData& y2ax = plotter_.axisData(2);
	const bool shwy2 = shwy2userdefpolyline_->isChecked();

	if ( shwy2 && !exp2plotted_ )
	{
	    y2ax.autoscalepars_.doautoscale_ = y2ax.needautoscale_ = true;
	    computePts( true ); exp2plotted_ = true;
	    line2drawn_ = err2bfrplot_= false;
	}
	else if ( !exp2plotted_ && mathexprstring1_.isEmpty() )
	{
	    y2ax.autoscalepars_.doautoscale_ = y2ax.needautoscale_ = true;
	    exp2plotted_ = true;
	}
	plotter_.setUserDefDrawType( shwy2,true );
    }
}


void computePts( bool isy2 )
{
    Math::Expression* mathobj = isy2 ? mathobj1_ : mathobj_;
    if ( !mathobj ) return;

    uiDataPointSetCrossPlotter::AxisData& horz = plotter_.axisData(0);
    uiDataPointSetCrossPlotter::AxisData& vert = plotter_.axisData(isy2 ? 2:1);
    vert.handleAutoScale( plotter_.uidps().getRunCalc( vert.colid_ ) );

    StepInterval<float> curvyvalrg( mUdf(float), -mUdf(float),
	    vert.axis_->range().step );
    const bool& linedrawn = isy2 ? line2drawn_ : line1drawn_;
    const int nrpts = linedrawn ? 70 : 1000;
    const Interval<float> xrge = horz.rg_;
    const float step = fabs( (xrge.stop-xrge.start)/(nrpts-1) );
    TypeSet<uiWorldPoint> pts;
    pts.setCapacity( nrpts, false );

    for ( int idx=0; idx<nrpts; idx++ )
    {
	const float curvxval = xrge.start + idx*step;
	mathobj->setVariableValue( 0, curvxval );
	const float curvyval = mCast(float,mathobj->getValue());
	if ( !Math::IsNormalNumber(curvyval) ) break;
	if ( mIsUdf(curvyval) || mIsUdf(curvxval) ) continue;

	curvyvalrg.include( curvyval, false );
	pts += uiWorldPoint( curvxval, curvyval );
    }

    if ( pts.size() == 0 )
    {
	msg_ = tr("Y%1 cannot be plotted").arg(isy2 ? 2 : 1);
	uiMSG().error( msg() );
	return;
    }

    if ( !vert.axis_->range().includes(curvyvalrg) )
    {
	msg_ = tr("Y%1 goes beyond the autoscaled range."
		  "\n\nDo you want to rescale?").arg(isy2 ? 2 : 1);

	if ( uiMSG().askGoOn(msg_) )
	{
	    curvyvalrg.include( vert.axis_->range(), false );
	    curvyvalrg.step = (curvyvalrg.stop - curvyvalrg.start)/4.0f;
	    vert.autoscalepars_.doautoscale_ = vert.needautoscale_ = false;
	    vert.axis_->setBounds( curvyvalrg );
	}
    }
    plotter_.setUserDefPolyLine( pts,isy2 );
}


void getRmsError( bool isy2 )
{
    uiDataPointSetCrossPlotter::AxisData& horz = plotter_.axisData(0);
    uiDataPointSetCrossPlotter::AxisData& vert = plotter_.axisData(isy2? 2:1);

    Math::Expression* mathobj = isy2 ? mathobj1_ : mathobj_;
    const BinIDValueSet& bvs = dps_.bivSet();
    BinIDValueSet::SPos pos;
    bool shwrmserr = true;
    double sqsumerr = 0.0;
    int count = 0;
    while ( bvs.next(pos,false) )
    {
	DataPointSet::RowID rid = dps_.getRowID( pos );
	const float xval = plotter_.getVal( horz.colid_, rid );
	const float yval = plotter_.getVal( vert.colid_, rid );
	if ( mIsUdf(xval) || mIsUdf(yval) ) continue;

	mathobj->setVariableValue( 0, xval );
	float expyval = mCast(float,mathobj->getValue());
	if ( !Math::IsNormalNumber(expyval) || mIsUdf(expyval) )
	{ shwrmserr = false; break; }

	const float sqerr = (expyval-yval)*(expyval-yval);
	if ( !Math::IsNormalNumber(sqerr) ) { shwrmserr = false; break; }
	sqsumerr += sqerr;
	if ( !Math::IsNormalNumber(sqsumerr) ) { shwrmserr = false; break; }
	count += 1;
    }

    if ( count != 0 && shwrmserr )
    {
	const double rmserr = Math::Sqrt(sqsumerr/count);
	isy2 ? rmsfld1_->setValue( rmserr ) : rmsfld_->setValue( rmserr );
	( isy2 ? plotter_.y2rmserr_ : plotter_.y1rmserr_ ) = rmserr;
	( isy2 ? err2bfrplot_ : err1bfrplot_ ) = true;
    }
    else
    {
	isy2 ? rmsfld1_->setText(0) : rmsfld_->setText(0);
	isy2 ? plotter_.y2rmserr_.setEmpty() : plotter_.y1rmserr_.setEmpty();
    }
}


void getRmsErrorCB( CallBacker* )
{
    if ( !drawlinefld_->isChecked() ) return;
    const bool drawy2 = selaxisfld_ && !selaxisfld_->getBoolValue();
    uiGenInput* inpfld = drawy2 ? inpfld1_ : inpfld_;
    if ( parseExp( inpfld ) ) getRmsError( drawy2 );
}


void setFlds( CallBacker* )
{
    if ( drawlinefld_->isChecked() )
    {
	const bool drawy2 = selaxisfld_ && !selaxisfld_->getBoolValue();
	bool& linedrawn = drawy2 ? line2drawn_ : line1drawn_;
	uiGenInput* inpfld = drawy2 ? inpfld1_ : inpfld_;
	const BufferString& userdefstr = drawy2 ? plotter_.userdefy2str_
						: plotter_.userdefy1str_;
	inpfld->setText( userdefstr );
	linedrawn = true;
    }
}


bool acceptOK()
{
    plotter_.userdefy1str_ = inpfld_->text();
    drawlinefld_->setChecked( false );

    if ( plotter_.userdefy1str_.isEmpty() )
    {
	mathexprstring_.setEmpty(); plotter_.y1rmserr_.setEmpty();
	shwy1userdefpolyline_->setChecked( false );
    }

    if ( hasy2_ )
    {
	plotter_.userdefy2str_ = inpfld1_->text();
	if ( plotter_.userdefy2str_.isEmpty() )
	{
	    mathexprstring1_.setEmpty(); plotter_.y2rmserr_.setEmpty();
	    shwy2userdefpolyline_->setChecked( false );
	}

	plotter_.setup().showy2userdefpolyline_
	    = shwy2userdefpolyline_->isChecked();
	if ( parseExp( inpfld1_ ) ) getRmsError( true );
    }
    plotter_.setup().showy1userdefpolyline_=shwy1userdefpolyline_->isChecked();
    if ( parseExp( inpfld_ ) ) getRmsError( false );

    drawPolyLines();
    return true;
}

    uiDataPointSetCrossPlotter&		plotter_;
    const DataPointSet&                 dps_;

    bool				hasy2_;
    bool				exp1plotted_;
    bool				exp2plotted_;
    bool				line1drawn_;
    bool				line2drawn_;
    bool				err1bfrplot_;
    bool				err2bfrplot_;
    uiGraphicsView::ODDragMode		dragmode_;
    uiGenInput*                         inpfld_;
    uiGenInput*                         inpfld1_;
    uiGenInput*				rmsfld_;
    uiGenInput*				rmsfld1_;
    uiGenInput*				selaxisfld_;
    uiCheckBox*				shwy1userdefpolyline_;
    uiCheckBox*				shwy2userdefpolyline_;
    uiCheckBox*				drawlinefld_;
    BufferString			mathexprstring_;
    BufferString			mathexprstring1_;
    Math::Expression*			mathobj_;
    Math::Expression*			mathobj1_;
    uiString		msg() const  { return msg_; }
    mutable uiString	msg_;
};


class uiDPSCPDisplayPropTab : public uiDlgGroup
{ mODTextTranslationClass(uiDPSCPDisplayPropTab);
public:

uiDPSCPDisplayPropTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),mJoinUiStrs(sDisplay(), sProperties()))
    , plotter_(p->plotter())
    , hasy2_(p->plotter().axisHandler(2))
{
    const MarkerStyle2D& mstyle = plotter_.setup().markerstyle_;
    sizefld_ = new uiGenInput(this,tr("Marker size"),IntInpSpec(mstyle.size_));
    uiStringSet shapenms;
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Square) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Circle) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Cross) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Plus) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Target) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::HLine) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::VLine) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Plane) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Triangle) );
    shapenms.add( MarkerStyle2D::toUiString(MarkerStyle2D::Arrow) );

    uiLabeledComboBox* llb =
	new uiLabeledComboBox( this, shapenms, tr("Marker shape") );
    shapefld_ = llb->box();
    llb->attach( alignedBelow, sizefld_ );
    shapefld_->setCurrentItem( (int)(mstyle.type_-1) );

    Color yaxiscol = plotter_.axisHandler(1)->setup().style_.color_;
    ycolinpfld_ = new uiColorInput( this, uiColorInput::Setup(yaxiscol)
		      .lbltxt(uiStrings::phrJoinStrings(uiStrings::sY(),
		      mJoinUiStrs(sAxis(), sColor()))));
    ycolinpfld_->attach( alignedBelow, llb );

    if ( hasy2_ )
    {
	Color y2axiscol = plotter_.axisHandler(2)->setup().style_.color_;
	y2colinpfld_ = new uiColorInput( this, uiColorInput::Setup(y2axiscol)
		       .lbltxt(uiStrings::phrJoinStrings(uiStrings::sY2(),
		       mJoinUiStrs(sAxis(), sColor()))));
	y2colinpfld_->attach( alignedBelow, ycolinpfld_ );
    }
}

bool acceptOK()
{
    if ( sizefld_->getIntValue() <= 0 )
    {
	uiMSG().error(tr("Cannot put negative size for size."));
	return false;
    }

    MarkerStyle2D& mstyle = plotter_.setup().markerstyle_;
    mstyle.size_ = sizefld_->getIntValue();
    mstyle.type_ = (MarkerStyle2D::Type)(shapefld_->currentItem()+1);
    plotter_.axisHandler(1)->setup().style_.color_ = ycolinpfld_->color();
    plotter_.axisHandler(1)->setup().gridlinestyle_.color_ =
							ycolinpfld_->color();
    if ( hasy2_ )
    {
	plotter_.axisHandler(2)->setup().style_.color_ = y2colinpfld_->color();
	plotter_.axisHandler(2)->setup().gridlinestyle_.color_ =
	    y2colinpfld_->color();
    }

    return true;
}

    uiDataPointSetCrossPlotter& plotter_;
    bool			hasy2_;
    uiGenInput*			sizefld_;
    uiComboBox*			shapefld_;
    uiColorInput*		ycolinpfld_;
    uiColorInput*		y2colinpfld_;
};


class uiDPSDensPlotSetTab : public uiDlgGroup
{ mODTextTranslationClass(uiDPSDensPlotSetTab);
public:

uiDPSDensPlotSetTab( uiDataPointSetCrossPlotterPropDlg* p )
    : uiDlgGroup(p->tabParent(),tr("Density Plot"))
    , plotter_(p->plotter())
{
    Settings& setts = Settings::common();
    if ( !setts.get(sKeyMinDPPts(),minptsfordensity_) )
	minptsfordensity_ = cMinPtsForDensity;
    uiString msg = tr("Current Number of Points "
		      "(including undefined values) %1")
		 .arg(plotter_.totalNrItems());
    uiLabel* lbl = new uiLabel( this, msg );

    const int cellsize = plotter_.cellSize();
    cellsize_ = cellsize;
    minptinpfld_ =
	new uiGenInput( this, tr("Threshold minimum points for Density Plot"),
	IntInpSpec(minptsfordensity_)
	.setLimits(StepInterval<int>(1,mCast(int,1e6),100)) );
    minptinpfld_->attach( rightAlignedBelow, lbl );

    cellsizefld_ = new uiGenInput(this, tr("Cell Size"), IntInpSpec(cellsize));
    cellsizefld_->attach( alignedBelow, minptinpfld_ );
    cellsizefld_->valuechanged.notify(
	    mCB(this,uiDPSDensPlotSetTab,cellSzChanged) );

    int width = 0;
    int height = 0;
    if ( plotter_.axisHandler(0) )
	width = plotter_.axisHandler(0)->pixRange().width();
    if ( plotter_.axisHandler(1) )
	height = plotter_.axisHandler(1)->pixRange().width();
    uiString  whcelltxt = tr("Nr of Cells across");
    wcellszfld_ = new uiGenInput( this, uiStrings::phrJoinStrings(whcelltxt,
		      uiStrings::sWidth()), IntInpSpec(width/cellsize) );
    wcellszfld_->attach( alignedBelow, cellsizefld_ );
    wcellszfld_->valuechanged.notify(
	    mCB(this,uiDPSDensPlotSetTab,wCellNrChanged) );
    hcellszfld_ = new uiGenInput( this, uiStrings::phrJoinStrings(whcelltxt,
		      uiStrings::sHeight()), IntInpSpec(height/cellsize) );
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

    wcellszfld_->setValue( mNINT32( (float)plotter_.arrArea()
                          .width()/cellsz) );
    hcellszfld_->setValue(
			mNINT32( (float) plotter_.arrArea().height()/cellsz) );
}

void wCellNrChanged( CallBacker* )
{
    const int cellsz = cellsizefld_->getIntValue();
    const float aspectratio = (float)(plotter_.arrArea().width()/cellsz)/
			      (float)(plotter_.arrArea().height()/cellsz);
    hcellszfld_->setValue( wcellszfld_->getIntValue()/aspectratio );
    cellsizefld_->setValue(
       mNINT32((float) plotter_.arrArea().width()/wcellszfld_->
       getIntValue()) );

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
	uiMSG().error(tr("Cannot have a cellsize less than 1"));
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
	: uiTabStackDlg( p->parent(),
			 uiDialog::Setup(uiStrings::sSettings(),
			 mNoDlgTitle,
			 mODHelpKey(mDataPointSetCrossPlotterPropDlgHelpID))
			 .modal(false) )
	, plotter_(*p)
	, bdroptab_(0)
{
    setDeleteOnClose( false );
    scaletab_ = new uiDPSCPScalingTab( this );
    addGroup( scaletab_ );
    statstab_ = new uiDPSCPStatsTab( this, true );
    addGroup( statstab_ );
    if ( plotter_.axisHandler(2) )
	addGroup( new uiDPSCPStatsTab(this,false) );

    userdeftab_ = new uiDPSUserDefTab( this );
    addGroup( userdeftab_ );
    dispproptab_ = new uiDPSCPDisplayPropTab( this );
    addGroup( dispproptab_ );
    densplottab_ = new uiDPSDensPlotSetTab( this );
    addGroup( densplottab_ );

    uiButton* applybut = uiButton::getStd( this, OD::Apply,
	    mCB(this,uiDataPointSetCrossPlotterPropDlg,doApply), true );
    applybut->attach( centeredBelow, tabObject() );
}


void uiDataPointSetCrossPlotterPropDlg::doApply( CallBacker* cb )
{
    acceptOK( cb );
}


bool uiDataPointSetCrossPlotterPropDlg::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<nrGroups(); idx++ )
	getGroup(idx).acceptOK();

    plotter_.dataChanged();
    return true;
}

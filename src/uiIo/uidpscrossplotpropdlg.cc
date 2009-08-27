/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jun 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidpscrossplotpropdlg.cc,v 1.10 2009-08-27 07:15:03 cvssatyaki Exp $";

#include "uidpscrossplotpropdlg.h"
#include "uidatapointsetcrossplot.h"

#include "linear.h"
#include "settings.h"

#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uicombobox.h"
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

    p->finaliseDone.notify( axselcb );
}

void axSel( CallBacker* )
{
    const int axnr = axselfld_->currentItem();
    if ( axnr < 0 ) return;

    for ( int idx=0; idx<3; idx++ )
    {
	uiDPSCPScalingTabAxFlds& axflds = *axflds_[idx];
	if ( !axflds.doclipfld_ ) continue;
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
	    float cr = axflds.percclipfld_->getfValue() * 0.01;
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

    p->finaliseDone.notify( mCB(this,uiDPSCPStatsTab,initFlds) );
}

void initFlds( CallBacker* )
{
    uiAxisHandler* xaxh = plotter_.axisHandler( 0 );
    uiAxisHandler* yaxh = plotter_.axisHandler( 1 );
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
    , hasy2_(plotter_.axisHandler(2))
    , shwy2userdefline_(0)
{
    uiLabel* y1lbl = new uiLabel( this, "Y1 =" );
    y1a0fld_ = new uiGenInput( this, "", FloatInpSpec(0) );
    y1a0fld_->attach( rightOf, y1lbl );
    uiLabel* y1pluslbl = new uiLabel( this, "+ " );
    y1pluslbl->attach( rightOf, y1a0fld_ );

    y1a1fld_ = new uiGenInput( this, "", FloatInpSpec(1) );
    y1a1fld_->attach( rightOf, y1pluslbl );
    uiLabel* y1xlbl = new uiLabel( this, "* X" );
    y1xlbl->attach( rightOf, y1a1fld_ );

    shwy1userdefline_ = new uiCheckBox( this, "Show Y1 User Defined line" );
    shwy1userdefline_->attach( alignedBelow, y1a0fld_ );
    
    if ( hasy2_ )
    {
	uiLabel* y2lbl = new uiLabel( this, "Y2 =" );
	y2lbl->attach( alignedBelow, y1lbl );
	y2lbl->attach( ensureBelow, shwy1userdefline_ );
	y2a0fld_ = new uiGenInput( this, "", FloatInpSpec(0) );
	y2a0fld_->attach( rightOf, y2lbl );
	uiLabel* y2pluslbl = new uiLabel( this, "+ " );
	y2pluslbl->attach( rightOf, y2a0fld_ );

	y2a1fld_ = new uiGenInput( this, "", FloatInpSpec(1) );
	y2a1fld_->attach( rightOf, y2pluslbl );
	uiLabel* y2xlbl = new uiLabel( this, "* X" );
	y2xlbl->attach( rightOf, y2a1fld_ );

	shwy2userdefline_ = new uiCheckBox( this, "Show Y2 User Defined line" );
	shwy2userdefline_->attach( alignedBelow, y2a0fld_ );
    }

    p->finaliseDone.notify( mCB(this,uiDPSUserDefTab,initFlds) );
}


void initFlds( CallBacker* )
{
    uiAxisHandler* xaxh = plotter_.axisHandler( 0 );
    uiAxisHandler* yaxh = plotter_.axisHandler( 1 );
    if ( !plotter_.axisHandler(0) || !plotter_.axisHandler(1) ) return;

    y1a0fld_->setValue( plotter_.userdefy1lp_.a0 );
    y1a1fld_->setValue( plotter_.userdefy1lp_.ax );
    
    if ( hasy2_ )
    {
	y2a0fld_->setValue( plotter_.userdefy2lp_.a0 );
	y2a1fld_->setValue( plotter_.userdefy2lp_.ax );
	shwy2userdefline_->setChecked( plotter_.setup().showy2userdefline_ );
    }
    shwy1userdefline_->setChecked( plotter_.setup().showy1userdefline_ );
}


bool acceptOK()
{
    plotter_.userdefy1lp_.a0 = y1a0fld_->getfValue();
    plotter_.userdefy1lp_.ax = y1a1fld_->getfValue();
    if ( !hasy2_ )
	plotter_.setup().showy2userdefline_ = false;
    else
    {
	plotter_.userdefy2lp_.a0 = y2a0fld_->getfValue();
	plotter_.userdefy2lp_.ax = y2a1fld_->getfValue();
	plotter_.setup().showy2userdefline_ = shwy2userdefline_->isChecked();
    }
    plotter_.setup().showy1userdefline_ = shwy1userdefline_->isChecked();
    return true;
}

    uiDataPointSetCrossPlotter&		plotter_;

    bool 		hasy2_;	
    uiGenInput*		y1a0fld_;
    uiGenInput*		y1a1fld_;
    uiGenInput*		y2a0fld_;
    uiGenInput*		y2a1fld_;
    uiCheckBox*		shwy1userdefline_;
    uiCheckBox*		shwy2userdefline_;

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
    BufferString msg( "Current Number of Points " );
    msg += plotter_.totalNrItems();
    uiLabel* lbl = new uiLabel( this, msg );
    minptinpfld_ = new uiGenInput( this, "Set minimum points for Density Plot",
	    			   IntInpSpec(minptsfordensity_) );
    minptinpfld_->attach( rightAlignedBelow, lbl );
}

bool acceptOK()
{
    minptsfordensity_ = minptinpfld_->getIntValue();
    Settings& setts = Settings::common();
    setts.set( sKeyMinDPPts(), minptsfordensity_ );
    return setts.write();
}

    uiDataPointSetCrossPlotter& plotter_;
    uiGenInput*			minptinpfld_;
    int				minptsfordensity_;
    static const char*		sKeyMinDPPts()
    				{ return "Minimum pts for Density Plot"; }
};


uiDataPointSetCrossPlotterPropDlg::uiDataPointSetCrossPlotterPropDlg(
		uiDataPointSetCrossPlotter* p )
	: uiTabStackDlg( p->parent(), uiDialog::Setup("Settings",0,mTODOHelpID))
	, plotter_(*p)
    	, bdroptab_(0)
{
    scaletab_ = new uiDPSCPScalingTab( this );
    addGroup( scaletab_ );
    statstab_ = new uiDPSCPStatsTab( this );
    addGroup( statstab_ );
    userdeftab_ = new uiDPSUserDefTab( this );
    addGroup( userdeftab_ );
    densplottab_ = new uiDPSDensPlotSetTab( this );
    addGroup( densplottab_ );
}


bool uiDataPointSetCrossPlotterPropDlg::acceptOK( CallBacker* )
{
    if ( scaletab_ ) scaletab_->acceptOK();
    if ( statstab_ ) statstab_->acceptOK();
    if ( userdeftab_ ) userdeftab_->acceptOK();
    if ( densplottab_ ) densplottab_->acceptOK();

    plotter_.dataChanged();
    return true;
}

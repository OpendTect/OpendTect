/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jun 2008
 RCS:           $Id: uidpscrossplotpropdlg.cc,v 1.1 2008-06-20 13:39:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidpscrossplotpropdlg.h"
#include "uidatapointsetcrossplot.h"

#include "uigeninput.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiaxishandler.h"
#include "uilineedit.h"
#include "uimsg.h"


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

	const uiDataPointSetCrossPlotter::AutoScalePars& asp
	    					= plotter_.autoScalePars(idx);
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

    finaliseDone.notify( axselcb );
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
	uiDataPointSetCrossPlotter::AutoScalePars& asp
	    			= plotter_.autoScalePars( idx );
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

	asp.doautoscale_ = doas;
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
    a0fld = new uiLineEdit( this, FloatInpSpec(0), "A0" );
    new uiLabel( this, "Y =", a0fld );
    a1fld = new uiLineEdit( this, FloatInpSpec(1), "A1" );
    new uiLabel( this, "+ ", a1fld );
    uiLabel* xlbl = new uiLabel( this, "* X", a0fld );
    xlbl->attach( rightOf, a1fld );
}

bool acceptOK()
{
    return true;
}

    uiDataPointSetCrossPlotter&		plotter_;
    uiLineEdit*	a0fld;
    uiLineEdit*	a1fld;

};


uiDataPointSetCrossPlotterPropDlg::uiDataPointSetCrossPlotterPropDlg(
		uiDataPointSetCrossPlotter* p )
	: uiTabStackDlg( p->parent(), uiDialog::Setup("Settings",0,"0.0.0") )
	, plotter_(*p)
    	, bdroptab_(0)
{
    addGroup( new uiDPSCPScalingTab(this) );
    addGroup( new uiDPSCPStatsTab(this) );
}


bool uiDataPointSetCrossPlotterPropDlg::acceptOK( CallBacker* )
{
    return true;
}

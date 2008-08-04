/*+
________________________________________________________________________

    CopyRight:     (C) dGB Beheer B.V.
    Author:        Nageswara
    Date:          May 2008
    RCS:           $Id: uimeasuredlg.cc,v 1.5 2008-08-04 11:11:31 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimeasuredlg.h"

#include "bufstring.h"
#include "draw.h"
#include "position.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uisellinest.h"


uiMeasureDlg::uiMeasureDlg( uiParent* p )
    : uiDialog( p, Setup("Measured Distance","","").modal(false) )
    , ls_(*new LineStyle)
    , appvelfld_(0)
    , lineStyleChange(this)
    , clearPressed(this)
{
    setOkText( "" );
    setCancelText( "" );

    uiGroup* topgrp = new uiGroup( this, "Info fields" );
    BufferString hdistlbl ( "Horizontal Distance ", SI().getXYUnit() );
    hdistfld_ = new uiGenInput( topgrp, hdistlbl, FloatInpSpec(0) );
    hdistfld_->setReadOnly( true );

    BufferString zdistlbl ( "Vertical Distance ", SI().getZUnit() ); 
    zdistfld_ = new uiGenInput( topgrp, zdistlbl, FloatInpSpec(0) );
    zdistfld_->setReadOnly( true );
    zdistfld_->attach( alignedBelow, hdistfld_ );

    if ( SI().zIsTime() )
    {
	BufferString lbl( "(", SI().getXYUnit(false), "/sec)" );
	BufferString vellbl( "Velocity ", lbl );
	appvelfld_ = new uiGenInput( topgrp, vellbl, FloatInpSpec(2000) );
	appvelfld_->valuechanged.notify( mCB(this,uiMeasureDlg,velocityChgd) );
	appvelfld_->attach( alignedBelow, zdistfld_ );
    }

    BufferString distlbl( "Distance ", SI().getXYUnit() );
    distfld_ = new uiGenInput( topgrp, distlbl, FloatInpSpec(0) );
    distfld_->setReadOnly( true );
    distfld_->attach( alignedBelow, appvelfld_ ? appvelfld_ : zdistfld_ );

    inlcrldistfld_ = new uiGenInput( topgrp, "Inl/Crl Distance",
	    			     IntInpIntervalSpec(Interval<int>(0,0))
				     .setName("InlDist",0)
				     .setName("CrlDist",1) );
    inlcrldistfld_->setReadOnly( true, -1 );
    inlcrldistfld_->attach( alignedBelow, distfld_ );

    uiGroup* botgrp = new uiGroup( this, "Button group" );
    uiPushButton* clearbut = new uiPushButton( botgrp, "&Clear",
				mCB(this,uiMeasureDlg,clearCB), true );
    uiPushButton* stylebut = new uiPushButton( botgrp, "&Line style",
				mCB(this,uiMeasureDlg,stylebutCB), false );
    stylebut->attach( rightTo, clearbut );

    botgrp->attach( centeredBelow, topgrp );
}


uiMeasureDlg::~uiMeasureDlg()
{
    delete &ls_;
}


void uiMeasureDlg::lsChangeCB( CallBacker* cb )
{
    ls_ = linestylefld_->getStyle();
    lineStyleChange.trigger( cb );
}


void uiMeasureDlg::clearCB( CallBacker* cb )
{ clearPressed.trigger( cb ); }


void uiMeasureDlg::stylebutCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup("","","") );
    dlg.setCtrlStyle( uiDialog::LeaveOnly );
    linestylefld_ = new uiSelLineStyle( &dlg, ls_, "", false, true, true );
    linestylefld_->changed.notify( mCB(this,uiMeasureDlg,lsChangeCB) );
    dlg.go();
}


void uiMeasureDlg::setLineStyle( const LineStyle& ls )
{ ls_ = ls; }


void uiMeasureDlg::velocityChgd( CallBacker* )
{
    pErrMsg( "Not implemented yet" );
}


void uiMeasureDlg::reset()
{
    hdistfld_->setValue( 0 );
    zdistfld_->setValue( 0 );
    if ( SI().zIsTime() )
    	appvelfld_->setValue( 2000 );
    distfld_->setValue( 0 );
    inlcrldistfld_->setValue( Interval<int>(0,0) );
}


static const double sM2Ft2 = 0.09290304;
static const double sFt2M2 = 10.76391;

void uiMeasureDlg::fill( TypeSet<Coord3>& points )
{
    const float velocity = SI().zIsTime() ? appvelfld_->getfValue() : 0 ;
    const int size = points.size();
    if ( size<2 )
    {
	reset();
	return;
    }

    int inldist = 0, crldist = 0;
    double hdist = 0, zdist = 0, realdist = 0;
    for ( int idx=1; idx<size; idx++ )
    {
	const Coord posxy = points[idx].coord();
	const Coord prevposxy = points[idx-1].coord();
	const BinID posbid = SI().transform( posxy );
	const BinID prevposbid = SI().transform( prevposxy );

	inldist += abs( posbid.r() - prevposbid.r() );
	crldist += abs( posbid.c() - prevposbid.c() );
	hdist += posxy.distTo( prevposxy );
	zdist += fabs( (points[idx].z - points[idx-1].z) * SI().zFactor() );
    
	double x1 = posxy.x;
	double x2 = prevposxy.x;
	double z1 = points[idx].z;
	double z2 = points[idx-1].z;
	if ( SI().zIsTime() )
	    realdist += sqrt( (x2-x1) * (x2-x1) +
		    	     (velocity*(z2-z1) * velocity*(z2-z1)) );
	else if ( SI().zInMeter() )
	{
	   if (  SI().xyInFeet() )
	       realdist += sqrt( (x2-x1) * (x2-x1) + sM2Ft2*((z2-z1)*(z2-z1)) );
	   else
	       realdist += sqrt( (x2-x1) * (x2-x1) +(z2-z1)*(z2-z1) );
	}
	else
	{
	    if (  SI().xyInFeet() )
		realdist += sqrt( (x2-x1) * (x2-x1) + (z2-z1)*(z2-z1) );
	    else
		 realdist += sqrt( sFt2M2*((x2-x1)*(x2-x1)) + (z2-z1)*(z2-z1) );
	}
    }

    hdistfld_->setValue( hdist );
    zdistfld_->setValue( zdist );
    distfld_->setValue( realdist );
    inlcrldistfld_->setValue( Interval<int>(inldist,crldist) );
}

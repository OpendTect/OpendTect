/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellto2dlinedlg.h"

#include "ui2dgeomman.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uiseisrandto2dline.h"
#include "uiwellrdmlinedlg.h"

#include "seisrandlineto2d.h"
#include "survgeometry.h"
#include "survinfo.h"

#include <math.h>

#include "od_helpids.h"

uiWellTo2DLineDlg::uiWellTo2DLineDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Create 2D line"),
				 tr("Select wells to set up the 2D line path"),
				 mODHelpKey(mWellto2DLineDlgHelpID)))
    , wantspreview_(this)
    , wellselgrp_(new uiWellSelGrp(this))
{
    createFields();
    attachFields();
}


uiWellTo2DLineDlg::~uiWellTo2DLineDlg()
{
    unRefPtr( rl_ );
}


void uiWellTo2DLineDlg::createFields()
{
    uiString txt = tr("Extend outward %1")
				       .arg(SI().getUiXYUnitString(true, true));
    float defdist = 100 * SI().inlDistance();
    extendfld_ = new uiGenInput( this, txt,
				FloatInpSpec(mCast(float,mNINT32(defdist))) );
    extendfld_->setWithCheck( true );
    extendfld_->setChecked( true );

    dispfld_ = new uiCheckBox( this, tr("Display 2D Line on creation") );
    dispfld_->setChecked( true );

    auto* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, extendfld_ );

    CallBack cb = mCB(this,uiWellTo2DLineDlg,previewPush);
    previewbutton_ = new uiPushButton( this, tr("Preview"), cb, true );
    previewbutton_->attach( ensureBelow, sep );

    rl_ = new Geometry::RandomLine;
    rl_->ref();
    randto2dlinefld_ = new uiSeisRandTo2DBase( this, false );

    linenmfld_ = new uiGenInput( this, uiStrings::sLineName(),
				 StringInpSpec("") );
}


void uiWellTo2DLineDlg::attachFields()
{
    randto2dlinefld_->attach( alignedBelow, previewbutton_ );
    extendfld_->attach( alignedBelow, wellselgrp_ );
    previewbutton_->attach( alignedBelow, extendfld_ );
    linenmfld_->attach( alignedBelow, randto2dlinefld_ );
    dispfld_->attach( alignedBelow, linenmfld_ );
}


bool uiWellTo2DLineDlg::dispOnCreation()
{
    return dispfld_->isChecked();
}


void uiWellTo2DLineDlg::getCoordinates( TypeSet<Coord>& coords )
{
    wellselgrp_->getCoordinates( coords );

    if ( extendfld_->isChecked() )
	extendLine( coords );
}


void uiWellTo2DLineDlg::extendLine( TypeSet<Coord>& coords )
{
    const int nrcoords = coords.size();
    if ( nrcoords < 1 ) return;
    float extradist = extendfld_->getFValue();
    if ( extradist < 0.1 || extradist > 1e6 ) return;
    if ( nrcoords == 1 )
    {
	const Coord c( coords[0] );
	coords.erase();
	coords += Coord( c.x-extradist, c.y );
	coords += c;
	coords += Coord( c.x+extradist, c.y );
    }
    else
    {
	TypeSet<Coord> oldcrds( coords );
	coords.erase();
	const Coord d0( oldcrds[1].x - oldcrds[0].x,
			oldcrds[1].y - oldcrds[0].y );
	float p = (float) Math::Sqrt( extradist * extradist / d0.sqAbs() );
	const Coord newc0( oldcrds[0].x - p * d0.x, oldcrds[0].y - p * d0.y );
	const Coord d1( oldcrds[nrcoords-1].x - oldcrds[nrcoords-2].x,
			oldcrds[nrcoords-1].y - oldcrds[nrcoords-2].y );
	p = (float) Math::Sqrt( extradist * extradist / d1.sqAbs() );
	const Coord newc1( oldcrds[nrcoords-1].x + p * d1.x,
			   oldcrds[nrcoords-1].y + p * d1.y );

	coords += newc0;
	for ( int idx=0; idx<oldcrds.size(); idx++ )
	    coords += oldcrds[idx];
	coords += newc1;
    }
}


void uiWellTo2DLineDlg::previewPush( CallBacker* cb )
{
    wantspreview_.trigger();
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }
bool uiWellTo2DLineDlg::acceptOK( CallBacker* )
{
    TypeSet<Coord> wellcoord; getCoordinates( wellcoord );
    if ( wellcoord.size() < 2 )
	mErrRet( tr("Please define at least two points") )

    for ( int idx=0; idx<wellcoord.size(); idx++ )
    {
	Coord c( wellcoord[idx] );
	if ( !SI().isInside(SI().transform(c),false) )
	{
	    Coord othcoord = wellcoord[idx ? idx - 1 : 1];
	    c = SurveyGeometry::getEdgePoint( othcoord, c );
	}
	rl_->addNode( SI().transform(c) );
    }
    BufferString linenm = linenmfld_->text();
    if ( linenm.isEmpty() )
    {
	mErrRet(tr("Please enter a %1")
		   .arg(uiStrings::sLineName()))
    }

    rl_->setName( linenm );
    if ( !randto2dlinefld_->checkInputs() )
	return false;

    Pos::GeomID geomid = Geom2DImpHandler::getGeomID( rl_->name() );
    if ( geomid.isUdf() )
	return false;

    SeisRandLineTo2D exec( *randto2dlinefld_->getInputIOObj(),
			   *randto2dlinefld_->getOutputIOObj(),
			    geomid, 1, *rl_ );
    uiTaskRunner dlg( this );
    if ( !TaskRunner::execute( &dlg, exec ) )
	return false;

    if ( !SI().has2D() )
	uiMSG().warning( tr("You need to change survey type to 'Both 2D and 3D'"
			 " to display the 2D line") );

    return true;
}


Pos::GeomID uiWellTo2DLineDlg::get2DLineID() const
{
    return Survey::GM().getGeomID( linenmfld_->text() );
}


const IOObj* uiWellTo2DLineDlg::get2DDataSetObj() const
{
    return randto2dlinefld_->getOutputIOObj();
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiimpexp2dgeom.h"

#include "uifileinput.h"
#include "uiioobjselgrp.h"
#include "uimsg.h"

#include "od_iostream.h"
#include "posinfo2d.h"
#include "survgeometrytransl.h"
#include "survgeom2d.h"


// TODO: Add uiImp2DGeom here


uiExp2DGeom::uiExp2DGeom( uiParent* p )
    : uiDialog(p,Setup(uiStrings::phrExport( tr("2D Geometry")),mNoDlgTitle,
		       mODHelpKey(mExp2DGeomHelpID)).modal(false))
{
    setOkText( uiStrings::sExport() );

    IOObjContext ctxt = mIOObjContext( SurvGeom2D );
    geomfld_ = new uiIOObjSelGrp( this, ctxt,
				  uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );

    outfld_ = new uiFileInput( this, uiStrings::sOutputFile(),
			       uiFileInput::Setup().forread(false) );
    outfld_->attach( alignedBelow, geomfld_ );
}


uiExp2DGeom::~uiExp2DGeom()
{
}


bool uiExp2DGeom::acceptOK( CallBacker* )
{
    const BufferString fnm = outfld_->fileName();
    if ( fnm.isEmpty() )
    {
	uiMSG().error( tr("Please enter the output file name") );
	return false;
    }

    od_ostream strm( fnm );
    if ( !strm.isOK() )
    {
	uiMSG().error( tr("Cannot open the output file") );
	return false;
    }

    BufferString outstr;
    TypeSet<MultiID> mids; geomfld_->getChosen( mids );
    for ( int gidx=0; gidx<mids.size(); gidx++ )
    {
	const Survey::Geometry* geom = Survey::GM().getGeometry( mids[gidx] );
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
	if ( !geom2d ) continue;

	const PosInfo::Line2DData& data2d = geom2d->data();
	const TypeSet<PosInfo::Line2DPos>& allpos = data2d.positions();

	for ( int pidx=0; pidx<allpos.size(); pidx++ )
	{
	    const PosInfo::Line2DPos& pos = allpos[pidx];
	    outstr.setEmpty();
	    const BufferString xcrdstr = toString(pos.coord_.x,2);
	    const BufferString ycrdstr = toString(pos.coord_.y,2);
	    outstr.add( data2d.lineName() ).addTab()
		  .add( pos.nr_ ).addTab()
		  .add( xcrdstr.buf() ).addTab()
		  .add( ycrdstr.buf() );
	    strm << outstr.buf() << '\n';
	}
    }

    strm.close();

    const uiString msg = tr("Geometry successfully exported.\n\n"
			    "Do you want to export more?");
    const bool res = uiMSG().askGoOn( msg );
    return !res;
}

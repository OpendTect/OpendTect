/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		April 2010
________________________________________________________________________

-*/

#include "hor2dfrom3dcreator.h"

#include "bufstring.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "dbkey.h"
#include "posinfo.h"
#include "ioobj.h"
#include "uistrings.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"

Hor2DFrom3DCreatorGrp::Hor2DFrom3DCreatorGrp( const EM::Horizon3D& hor3d,
					      EM::Horizon2D& hor2d )
    : ExecutorGroup( "Create 2D Horizon from 3D" )
    , hor3d_(hor3d)
    , hor2d_(hor2d)
{
    hor3d_.ref();
    hor2d_.ref();
}


Hor2DFrom3DCreatorGrp::~Hor2DFrom3DCreatorGrp()
{
    hor3d_.unRef();
    hor2d_.unRef();
}


void Hor2DFrom3DCreatorGrp::init( const GeomIDSet& linegeomids )
{
    for ( int idx=0; idx<linegeomids.size(); idx++ )
    {
	Hor2DFrom3DCreator* creator = new Hor2DFrom3DCreator( hor3d_, hor2d_ );
	if ( !creator->setCreator(linegeomids[idx]) )
	    continue;

	add( creator );
    }
}


Hor2DFrom3DCreator::Hor2DFrom3DCreator( const EM::Horizon3D& hor3d,
					EM::Horizon2D& hor2d )
    : Executor("Creating 2D horizon from 3D" )
    , hor3d_(hor3d)
    , hor2d_(hor2d)
    , geom2d_(0)
    , nrdone_(0)
{
}


bool Hor2DFrom3DCreator::setCreator( Pos::GeomID geomid )
{
    const auto& geom2d = SurvGeom::get2D( geomid );
    if ( geom2d.isEmpty() )
	return false;

    geomid_ = geomid;
    geom2d_ = &geom2d;
    hor2d_.geometry().addLine( geomid_ );
    totalnr_ = geom2d_->data().positions().size();
    return true;
}


uiString Hor2DFrom3DCreator::message() const
{ return uiStrings::phrHandling(uiStrings::sPosition(mPlural)); }
uiString Hor2DFrom3DCreator::nrDoneText() const
{ return uiStrings::phrHandled(uiStrings::sPosition(mPlural)); }


int Hor2DFrom3DCreator::nextStep()
{
    if ( !geom2d_ )
	return ErrorOccurred();
    else if ( nrdone_ >= totalnr_ )
	return Finished();

    const PosInfo::Line2DPos& posinfo = geom2d_->data().positions()[nrdone_];
    BinID bid = SI().transform( posinfo.coord_ );
    const float zval = hor3d_.getZ( bid );
    hor2d_.setZPos( geomid_, posinfo.nr_, zval, false );
    nrdone_++;
    return MoreToDo();
}

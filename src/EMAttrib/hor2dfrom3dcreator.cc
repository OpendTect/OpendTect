/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "hor2dfrom3dcreator.h"

#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "survinfo.h"
#include "survgeom2d.h"

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


void Hor2DFrom3DCreatorGrp::init( const TypeSet<Pos::GeomID>& linegeomids )
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
    , geomid_(Survey::GeometryManager::cUndefGeomID())
    , geom2d_(0)
    , nrdone_(0)
{
}


Hor2DFrom3DCreator::~Hor2DFrom3DCreator()
{}


bool Hor2DFrom3DCreator::setCreator( Pos::GeomID geomid )
{
    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
	    	     Survey::GM().getGeometry(geomid) );
    if ( !geom2d )
	return false;

    geomid_ = geomid;
    geom2d_ = geom2d;
    hor2d_.geometry().addLine( geomid_ );
    totalnr_ = geom2d_->data().positions().size();
    return true;
}


int Hor2DFrom3DCreator::nextStep()
{
    if ( !geom2d_ )
	return ErrorOccurred();
    else if ( nrdone_ >= totalnr_ )
	return Finished();

    const PosInfo::Line2DPos& posinfo = geom2d_->data().positions()[nrdone_];
    const BinID bid = SI().transform( posinfo.coord_ );
    const float horz = hor3d_.getZ( bid );
    hor2d_.setPos( geomid_, posinfo.nr_, horz,false);
    nrdone_++;
    return MoreToDo();
}

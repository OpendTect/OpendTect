/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		April 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: hor2dfrom3dcreator.cc,v 1.5 2010-09-29 03:48:48 cvssatyaki Exp $";

#include "hor2dfrom3dcreator.h"

#include "bufstring.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "multiid.h"
#include "posinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "surv2dgeom.h"

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


void Hor2DFrom3DCreatorGrp::init( const BufferStringSet& linenames,
				  const MultiID& key )
{
    for ( int idx=0; idx<linenames.size(); idx++ )
    {
	Hor2DFrom3DCreator* creator = new Hor2DFrom3DCreator( hor3d_, hor2d_ );
	if ( !creator->setCreator(linenames.get(idx),key) )
	    continue;
	add( creator );
    }
}


Hor2DFrom3DCreator::Hor2DFrom3DCreator( const EM::Horizon3D& hor3d,
					EM::Horizon2D& hor2d )
    : Executor("Creating 2D horizon from 3D" )
    , hor3d_(hor3d)
    , hor2d_(hor2d)
    , nrdone_(0)
{
}


bool Hor2DFrom3DCreator::setCreator( const BufferString& linename,
				     const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj ) return false;

    BufferString fnm = ioobj->fullUserExpr(true);
    Seis2DLineSet lineset( fnm );
    int lineidx = lineset.indexOf( linename );
    if ( lineidx<0 ) return false;

    posdata_.setLineName( linename );
    PosInfo::POS2DAdmin().setCurLineSet( lineset.name() );
    PosInfo::POS2DAdmin().getGeometry( posdata_ );
    lineid_ = hor2d_.geometry().addLine( key, linename );
    hor2d_.geometry().syncLine( key, linename, posdata_ );
    totalnr_ = posdata_.positions().size();
    return true;
}


int Hor2DFrom3DCreator::nextStep()
{
    if ( nrdone_ < totalnr_ )
    {
	const PosInfo::Line2DPos& posinfo = posdata_.positions()[nrdone_];
	BinID bid = SI().transform( posinfo.coord_ );
	EM::SubID subid = bid.toInt64();
	const Coord3 pos3d = hor3d_.getPos( hor3d_.sectionID(0), subid );
	subid = RowCol( lineid_, posinfo.nr_ ).toInt64();
	hor2d_.setPos(hor2d_.sectionID(0),subid,pos3d,false);
	nrdone_++;
	return MoreToDo();
    }

    return Finished();
}

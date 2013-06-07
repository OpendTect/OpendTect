/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";

#include "faultextender.h"

#include "cubicbeziersurface.h"
#include "emfault3d.h"
#include "survinfo.h"

namespace MPE 
{


FaultExtender::FaultExtender( EM::Fault3D& flt, const EM::SectionID& sid )
    : SectionExtender(sid)
    , fault(flt)
    , maxdistance(Coord3::udf())
{}


const Coord3& FaultExtender::maxDistance() const { return maxdistance; }


void FaultExtender::setMaxDistance( const Coord3& nm )
{ maxdistance = nm; }


void FaultExtender::setDirection( const BinIDValue& bdval )
{ direction =  bdval; }


int FaultExtender::nextStep()
{
    return 0;
    /*
    TypeSet<RowCol> rcs;
    for ( int idx=0; idx<startpos.size(); idx++ )
	rcs += RowCol( startpos[idx] );

    if ( rcs.isEmpty() )
	return 0;

    mDynamicCastGet(const Geometry::CubicBezierSurface*,surface,
	    const_cast<const EM::Fault&>(fault).sectionGeometry(sid));

    const_cast<Geometry::CubicBezierSurface*>(surface)->trimUndefParts();    

    StepInterval<int> rowrange = surface->rowRange();
    StepInterval<int> colrange = surface->colRange();

    const Coord3 draggerdir =
	Coord3( SI().transform(direction.binid)-SI().transform(BinID(0,0)),
		direction.value).normalize();

    const bool rowdir = !direction.binid.inl && !direction.binid.crl;
    char inc;
    if ( rowdir ) inc = direction.value>0 ? rowrange.step : -rowrange.step;
    else if ( colrange.start==colrange.stop ) inc = colrange.step;
    else
    {
	//Check coldir for all rcs, remove rcs that cannot grow, and determine
	//if its cols should increase or decrease
	int nrpos = 0;
	int nrneg = 0;
	for ( int idx=0; idx<rcs.size(); idx++ )
	{
	    Coord3 coldir = surface->getColDirection(rcs[idx], true );
	    if ( coldir.isDefined() )
	    {
		coldir = coldir.normalize();
		static float cos60 = 0.5;
		const float cosangle = coldir.dot(draggerdir);
		if ( fabs(cosangle)<cos60 ) { rcs.remove(idx--); continue; }

		if ( cosangle>0 ) nrpos++;
		else nrneg++;
	    }
	}

	inc = nrpos>=nrneg ? colrange.step : -colrange.step;
    }

    TypeSet<int> linekeys;
    for ( int idx=0; idx<rcs.size(); idx++ )
    {
	const int key = rowdir ? rcs[idx].row : rcs[idx].col;
	if ( linekeys.indexOf(key)==-1 ) linekeys += key;
    }


    const bool duplicate = rowdir
	? rowrange.start==rowrange.stop : colrange.start==colrange.stop;

    for ( int idx=0; idx<linekeys.size(); idx++ )
    {
	TypeSet<RowCol> linercs;
	if ( linekeys.size()==1 ) linercs = rcs;
	else
	{
	    for ( int idy=0; idy<rcs.size(); idy++ )
	    {
		if ( (rowdir&&rcs[idy].row!=linekeys[idx]) ||  
		     ( !rowdir && rcs[idy].col!=linekeys[idx]) )  
		    continue;

		linercs += rcs[idy];
	    }
	}

	Coord3 growthdir;
	if ( duplicate ) growthdir = draggerdir;
	else
	{
	    growthdir = Coord3(0,0,0);
	    int nrdirs = 0;
	    for ( int idy=0; idy<linercs.size(); idy++ )
	    {
		const Coord3 localgrowthdir = rowdir
		    ? surface->getRowDirection(linercs[idy], true )
		    : surface->getColDirection(linercs[idy], true );

		if ( !localgrowthdir.isDefined() ) continue;
		if ( localgrowthdir.dot(draggerdir)>0!=inc>0 )
		{ linercs.remove(idy--); continue; }
		
		const RowCol& rc = linercs[idy];
		const RowCol nextrc = rowdir
		    ? RowCol(rc.row+inc,rc.col) : RowCol(rc.row, rc.col+inc );

		if ( surface->isKnotDefined(nextrc) )
		{ linercs.remove(idy--); continue; }

		growthdir += localgrowthdir.normalize();
		nrdirs++;
	    }

	    if ( !nrdirs ) growthdir = draggerdir.normalize();
	    else growthdir = growthdir*inc/nrdirs;
	}

	// TODO: Estimate better growthdir
	// ( can't normalize localgrowthdir when dragging up and down )
	if ( !draggerdir.x && !draggerdir.y )
	    growthdir = draggerdir;

	const Coord relcrd = SI().transform( direction.binid ) - 
	    		     SI().transform( BinID(0,0) );
	const Coord3 dirvector( relcrd, direction.value );

	TypeSet<RowCol> changednodes;
	for ( int idy=0; idy<linercs.size(); idy++ )
	{
	    const RowCol& rc( linercs[idy] );
	    RowCol targetrc = rc;
	    if ( duplicate )
	    {
		targetrc.row += rowdir ? inc : 0;
		targetrc.col += rowdir ? 0 : inc;
	    }

	    const Coord3 newpos = surface->getKnot(rc) +
						   growthdir*dirvector.abs();
	    fault.setPos( sid, targetrc.toInt64(), newpos, true );

	    changednodes += targetrc;
	}

	for ( int idy=0; idy<changednodes.size(); idy++ )
	{
	    const RowCol rc( changednodes[idx] );
	    const RowCol backnode(rc.row-(rowdir?inc:0), rc.col-(rowdir?0:inc));

	    const Coord3 backcoord = surface->getKnot(backnode);
	    if ( !backcoord.isDefined() )
		continue;

	    const Coord3 diff = backcoord-surface->getKnot(rc);

	    if ( (!mIsUdf(maxdistance.x) && maxdistance.x<fabs(diff.x)) ||
		 (!mIsUdf(maxdistance.y) && maxdistance.y<fabs(diff.y)) ||
		 (!mIsUdf(maxdistance.z) && maxdistance.z<fabs(diff.z)) )
	    {
		if ( rowdir )
		{
		    const int newrow =  mMAX(rc.row,backnode.row);
		    if ( !fault.geometry.insertRow(sid,newrow,true) )
		    {
			pErrMsg("Hue");
			continue;
		    }

		    for ( int idz=0; idz<rcs.size(); idz++ )
		    {
			if ( rcs[idz].row>=newrow )
			    rcs[idz].row += rowrange.step;
		    }

		    for ( int idz=0; idz<changednodes.size(); idz++ )
		    {
			if ( changednodes[idz].row>=newrow )
			    changednodes[idz].row += rowrange.step;
		    }

		    for ( int idz=0; idz<addedpos.size(); idz++ )
		    {
			RowCol addedrc( addedpos[idz] );
			if ( addedrc.row>=newrow )
			{
			    addedrc.row += rowrange.step;
			    addedpos[idz] = addedrc.toInt64();
			}
		    }

		    for ( int idz=colrange.start; idz<=colrange.stop;
			  idz+=colrange.step )
		    {
			const RowCol addrc( newrow, idz );
			if ( surface->isKnotDefined(addrc) )
			    changednodes += addrc;
		    }
		}
		else
		{
		    const int newcol =  mMAX(rc.col,backnode.col);
		    if ( !fault.geometry.insertCol(sid,newcol,true) )
		    {
			pErrMsg("Hue");
			continue;
		    }
		
		    // Update local rowcol lists to the inserted column	
		    for ( int idz=0; idz<rcs.size(); idz++ )
		    {
			if ( rcs[idz].col>=newcol )
			    rcs[idz].col += colrange.step;
		    }

		    for ( int idz=0; idz<changednodes.size(); idz++ )
		    {
			if ( changednodes[idz].col>=newcol )
			    changednodes[idz].col += colrange.step;
		    }

		    for ( int idz=0; idz<addedpos.size(); idz++ )
		    {
			RowCol addedrc( addedpos[idz] );
			if ( addedrc.col>=newcol )
			{
			    addedrc.col += colrange.step;
			    addedpos[idz] = addedrc.toInt64();
			}
		    }

		    for ( int idz=rowrange.start; idz<=rowrange.stop;
			  idz+=rowrange.step )
		    {
			const RowCol addrc( idz, newcol );
			if ( surface->isKnotDefined(addrc)
			     && changednodes.indexOf(addrc)==-1  )
			    changednodes += addrc;
		    }
		}

		idy = -1; //restart checks
	    }
	}

	for ( int idy=0; idy<changednodes.size(); idy++ )
	{
	    const GeomPosID pid = changednodes[idy].toInt64();
	    if ( addedpos.indexOf(pid)==-1 )
		addTarget(pid, rcs[idx].toInt64() );
	}
    }

    return 0;
    */
}


};  //namespace

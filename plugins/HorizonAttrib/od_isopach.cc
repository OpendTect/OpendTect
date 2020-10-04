/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		June 2011
 SVN:		$Id$
________________________________________________________________________

-*/

#include "batchprog.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "executor.h"
#include "isopachmaker.h"
#include "multiid.h"
#include "survinfo.h"
#include "moddepmgr.h"

static bool loadHorizon( const MultiID& mid, od_ostream& strm )
{
    EM::EMManager& em = EM::EMM();
    strm << "Loading horizon '" << em.objectName( mid ) << "'" << od_newline;
    Executor* exec = em.objectLoader( mid );
    if ( !(exec && exec->go( strm, false, false, 0 )) )
    {
	strm << "Failed to load horizon: ";
	strm << em.objectName( mid ).buf() << od_newline;
	return false;
    }

    return true;
}


mLoad1Module("EarthModel")
{
    strm << "Loading Horizons ..." << od_newline;
    MultiID mid1;
    pars().get( IsochronMaker::sKeyHorizonID(), mid1 );
    if ( !loadHorizon( mid1, strm ) )
	return false;

    EM::EMManager& em = EM::EMM();
    EM::ObjectID emid1 = em.getObjectID( mid1 );
    EM::EMObject* emobj1 = em.getObject( emid1 );
    mDynamicCastGet(EM::Horizon3D*,horizon1,emobj1);
    if ( !horizon1 )
	return false;

    MultiID mid2;
    pars().get( IsochronMaker::sKeyCalculateToHorID(), mid2 );
    if ( !loadHorizon( mid2, strm ) )
	return false;

    EM::ObjectID emid2 = em.getObjectID( mid2 );
    EM::EMObject* emobj2 = em.getObject( emid2 );
    mDynamicCastGet(EM::Horizon3D*,horizon2,emobj2)
    if ( !horizon2 )
	return false;

    strm << "Horizons successfully loaded" << od_newline;
    horizon1->ref();
    horizon2->ref();

    BufferString attrnm;
    pars().get( IsochronMaker::sKeyAttribName(), attrnm );
    if ( attrnm.isEmpty() )
    {
	horizon1->unRef();horizon2->unRef();
	return false;
    }

    int dataidx = horizon1->auxdata.auxDataIndex( attrnm );
    if ( dataidx < 0 )
	dataidx = horizon1->auxdata.addAuxData( attrnm );

    strm << "Calculating Isochron ..." << od_newline;
    IsochronMaker maker( *horizon1, *horizon2, attrnm, dataidx );
    if ( SI().zIsTime() )
    {
	bool isinmsec = false;
	pars().getYN( IsochronMaker::sKeyOutputInMilliSecYN(), isinmsec );
	maker.setUnits( isinmsec );
    }

    if ( !maker.go( strm, false, false, 0 ) )
    {
	strm << "Failed to calculate Isochron" << od_newline;
	horizon1->unRef(); horizon2->unRef();
	return false;
    }

    strm << "Isochron '" << attrnm.buf() << "' calculated successfully\n";
    strm << "Saving Isochron Attribute ..." << od_newline;
    bool isoverwrite = false;
    pars().getYN( IsochronMaker::sKeyIsOverWriteYN(), isoverwrite );
    if ( !maker.saveAttribute( horizon1, dataidx, isoverwrite, &strm ) )
    {
	strm << "Failed to save Isochron Attribute" << od_newline;
	horizon1->unRef(); horizon2->unRef();
	return false;
    }

    strm << "Isochron '" << attrnm.buf() << "' saved successfully"
	 << od_newline;
    horizon1->unRef(); horizon2->unRef();
    return true;
}

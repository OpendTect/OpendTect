/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		June 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "batchprog.h"

#include "attribstorprovider.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurface.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "genc.h"
#include "iopar.h"
#include "multiid.h"
#include "stratamp.h"
#include "stattype.h"
#include "moddepmgr.h"

#include <iostream>


static bool getHorsampling( const IOPar& par, HorSampling& hs )
{
    BufferString compkey = IOPar::compKey( sKey::Output(), sKey::Subsel() );
    const IOPar* hspar = par.subselect( compkey );
    if ( !hspar ) return false;

    return hs.usePar( *hspar ) ? true : false;
}


static EM::Horizon3D* loadHorizon( const MultiID& mid, const HorSampling& hs,
				   std::ostream& strm )
{
    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    sdsel.rg = hs;
    strm << "Laoding " << em.objectName( mid ) << std::endl;
    Executor* exec = em.objectLoader( mid, &sdsel );
    if ( !(exec && exec->execute(&strm, false, false, 0) ) )
	return 0;

    EM::ObjectID emid = em.getObjectID( mid );
    EM::EMObject* emobj = em.getObject( emid );
    if ( !emobj )
    {
	BufferString msg;
	msg = "Error while loading horizon '";
	msg.add( em.objectName( mid ) ).add( "'" );
	strm << msg << std::endl;
	return 0;
    }

    emobj->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    return horizon;
}


#define mUnRef(){ tophor->unRef(); if ( bothor ) bothor->unRef(); }

bool BatchProgram::go( std::ostream& strm )
{
    OD::ModDeps().ensureLoaded( "EMAttrib" );
    OD::ModDeps().ensureLoaded( "PreStackProcessing" );
    OD::ModDeps().ensureLoaded( "Attributes" );

    HorSampling hs;
    if ( !getHorsampling( pars(), hs ) )
	return false;

    bool usesingle = false;
    pars().getYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle );
    MultiID mid1;
    pars().get( StratAmpCalc::sKeyTopHorizonID(), mid1 );
    strm << GetProjectVersionName() << std::endl;
    strm << "Loading horizons ..." << std::endl;
    EM::Horizon3D* tophor = loadHorizon( mid1, hs, strm );
    if ( !tophor )
	return false;

    EM::Horizon3D* bothor = 0;
    if ( !usesingle )
    {
	MultiID mid2;
	pars().get( StratAmpCalc::sKeyBottomHorizonID(), mid2 );
	bothor = loadHorizon( mid2, hs, strm );
	if ( !bothor )
	{
	    tophor->unRef();
	    return false;
	}
    }

    strm << "Horizon(s) loaded successfully" << std::endl;

    BufferString type;
    pars().get( StratAmpCalc::sKeyAmplitudeOption(), type );
    Stats::Type sttype = Stats::parseEnumType( type );
    bool outputfold = false;
    pars().getYN( StratAmpCalc::sKeyOutputFoldYN(), outputfold );
    StratAmpCalc exec( tophor, usesingle ? 0 : bothor, sttype, hs, outputfold );
    int attribidx = exec.init( pars() );
    if ( attribidx < 0 )
    {
	strm << "Cannot add attribute to Horizon" << std::endl;
	mUnRef();
	return false;
    }

    strm << "Calculating attribute ..." << std::endl;
    if ( !exec.execute( &strm, false, false, 0 ) )
    {
	strm << "Failed to calculate attribute." << std::endl;
	mUnRef();
	return false;
    }

    infoMsg( "Attribute calculated successfully\n" );
    strm << "Saving attribute..." << std::endl;
    bool addtotop = false;
    pars().getYN( StratAmpCalc::sKeyAddToTopYN(), addtotop );
    bool isoverwrite = false;
    pars().getYN( StratAmpCalc::sKeyIsOverwriteYN(), isoverwrite );
    if ( !exec.saveAttribute( addtotop ? tophor : bothor, attribidx,
			      isoverwrite, &strm ) )
    {
	strm << "Failed to save attribute";
	mUnRef();
	return false;
    }
    
    strm << "Attribute saved successfully";
    mUnRef();
    return true;
}

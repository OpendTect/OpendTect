/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "genc.h"
#include "iopar.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "multiid.h"
#include "stratamp.h"
#include "stattype.h"

#include <iostream>


static bool getHorsampling( const IOPar& par, TrcKeySampling& hs )
{
    BufferString compkey = IOPar::compKey( sKey::Output(), sKey::Subsel() );
    const IOPar* hspar = par.subselect( compkey );
    if ( !hspar )
	return false;

    return hs.usePar( *hspar ) ? true : false;
}


static ConstRefMan<EM::Horizon3D> loadHorizon( const MultiID& mid,
					       const TrcKeySampling& hs,
					       od_ostream& strm )
{
    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    sdsel.rg = hs;
    strm << "Loading " << em.objectName( mid ) << od_newline;
    Executor* exec = em.objectLoader( mid, &sdsel );
    TextTaskRunner taskr( strm );
    if ( !exec || !taskr.execute(*exec) )
	return nullptr;

    EM::ObjectID emid = em.getObjectID( mid );
    ConstRefMan<EM::EMObject> emobj = em.getObject( emid );
    if ( !emobj )
    {
	BufferString msg;
	msg = "Error while loading horizon '";
	msg.add( em.objectName( mid ) ).add( "'" );
	strm << msg << od_newline;
	return nullptr;
    }

    ConstRefMan<EM::Horizon3D> horizon = dCast( const EM::Horizon3D*,
						emobj.ptr() );
    return horizon;
}


mLoad3Modules("EMAttrib","WellAttrib","PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    TrcKeySampling hs;
    if ( !getHorsampling( pars(), hs ) )
	return false;

    bool usesingle = false;
    pars().getYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle );
    MultiID mid1;
    pars().get( StratAmpCalc::sKeyTopHorizonID(), mid1 );
    strm << GetProjectVersionName() << od_newline;
    strm << "Loading horizons ..." << od_newline;
    ConstRefMan<EM::Horizon3D> tophor = loadHorizon( mid1, hs, strm );
    if ( !tophor )
	return false;

    ConstRefMan<EM::Horizon3D> bothor;
    if ( !usesingle )
    {
	MultiID mid2;
	pars().get( StratAmpCalc::sKeyBottomHorizonID(), mid2 );
	bothor = loadHorizon( mid2, hs, strm );
	if ( !bothor )
	    return false;
    }

    strm << "Horizon(s) loaded successfully" << od_newline;

    BufferString type;
    pars().get( StratAmpCalc::sKeyAmplitudeOption(), type );
    Stats::Type sttype = Stats::parseEnumType( type );
    bool outputfold = false;
    pars().getYN( StratAmpCalc::sKeyOutputFoldYN(), outputfold );
    StratAmpCalc exec( tophor.ptr(), usesingle ? nullptr : bothor.ptr(),
		       sttype, hs, outputfold );
    if ( !exec.doInit(pars()) )
    {
	strm << "Cannot add attribute to Horizon" << od_newline;
	return false;
    }

    strm << "Calculating attribute ..." << od_newline;
    TextTaskRunner taskr( strm );
    if ( !taskr.execute(exec) )
    {
	strm << "Failed to calculate attribute." << od_newline;
	return false;
    }

    infoMsg( "Attribute calculated successfully\n" );
    strm << "Saving attribute..." << od_newline;
    bool addtotop = false;
    pars().getYN( StratAmpCalc::sKeyAddToTopYN(), addtotop );
    bool isoverwrite = false;
    pars().getYN( StratAmpCalc::sKeyIsOverwriteYN(), isoverwrite );
    const TypeSet<int>& attribidxs = exec.attribIdxs();
    const TypeSet<int>& foldidxs = exec.foldAttribIdxs();
    for ( int idx=0; idx<attribidxs.size(); idx++ )
    {
	const int attribidx = attribidxs[idx];
	const int foldidx = exec.doOutputFold() ? foldidxs[idx] : -1;
	if ( !exec.doSaveAttribute( addtotop ? *tophor : *bothor, attribidx,
				  isoverwrite, foldidx, &strm ) )
	{
	    strm << "Failed to save attribute";
	    return false;
	}
    }
    
    strm << "Attribute saved successfully";
    return true;
}

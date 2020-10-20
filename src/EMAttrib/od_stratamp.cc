/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		June 2011
________________________________________________________________________

-*/

#include "batchprog.h"

#include "attribstorprovider.h"
#include "dbkey.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurface.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "genc.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "stratamp.h"
#include "stattype.h"
#include "trckeysampling.h"

#include <iostream>


static bool getHorsampling( const IOPar& par, TrcKeySampling& hs )
{
    BufferString compkey = IOPar::compKey( sKey::Output(), sKey::Subsel() );
    const IOPar* hspar = par.subselect( compkey );
    if ( !hspar ) return false;

    return hs.usePar( *hspar ) ? true : false;
}


static EM::Horizon3D* loadHorizon( const DBKey& dbky, const TrcKeySampling& hs,
				   od_ostream& strm )
{
    EM::ObjectManager& mgr = EM::Hor3DMan();
    strm << "Loading " << mgr.objectName( dbky ) << od_newline;
    LoggedTaskRunnerProvider trprov( strm );
    RefMan<EM::Object> emobj = mgr.fetchForEdit( dbky, trprov );
    if ( !emobj )
    {
	BufferString msg;
	msg = "Error while loading horizon '";
	msg.add( mgr.objectName( dbky ) ).add( "'" );
	strm << msg << od_newline;
	return 0;
    }

    emobj->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj.ptr())
    return horizon;
}


#define mUnRef(){ tophor->unRef(); if ( bothor ) bothor->unRef(); }

mLoad3Modules("Attributes","EMAttrib","PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    TrcKeySampling hs;
    if ( !getHorsampling( pars(), hs ) )
	return false;

    bool usesingle = false;
    pars().getYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle );
    DBKey dbky1;
    pars().get( StratAmpCalc::sKeyTopHorizonID(), dbky1 );
    strm << GetProjectVersionName() << od_newline;
    strm << "Loading horizons ..." << od_newline;
    EM::Horizon3D* tophor = loadHorizon( dbky1, hs, strm );
    if ( !tophor )
	return false;

    EM::Horizon3D* bothor = 0;
    if ( !usesingle )
    {
	DBKey dbky2;
	pars().get( StratAmpCalc::sKeyBottomHorizonID(), dbky2 );
	bothor = loadHorizon( dbky2, hs, strm );
	if ( !bothor )
	{
	    tophor->unRef();
	    return false;
	}
    }

    strm << "Horizon(s) loaded successfully" << od_newline;

    BufferString type;
    pars().get( StratAmpCalc::sKeyAmplitudeOption(), type );
    Stats::Type sttype = Stats::TypeDef().parse( type );
    bool outputfold = false;
    pars().getYN( StratAmpCalc::sKeyOutputFoldYN(), outputfold );
    StratAmpCalc exec( tophor, usesingle ? 0 : bothor, sttype, hs, outputfold );
    int attribidx = exec.init( pars() );
    if ( attribidx < 0 )
    {
	strm << "Cannot add attribute to Horizon" << od_newline;
	mUnRef();
	return false;
    }

    strm << "Calculating attribute ..." << od_newline;
    if ( !exec.go( strm, false, false, 0 ) )
    {
	strm << "Failed to calculate attribute." << od_newline;
	mUnRef();
	return false;
    }

    infoMsg( "Attribute calculated successfully\n" );
    strm << "Saving attribute..." << od_newline;
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

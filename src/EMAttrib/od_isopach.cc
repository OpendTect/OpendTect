/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		June 2011
________________________________________________________________________

-*/

#include "batchprog.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "executor.h"
#include "isopachmaker.h"
#include "dbkey.h"
#include "survinfo.h"
#include "moddepmgr.h"

static const char* getIsoMapType()
{ return SI().zIsTime() ? "Isochron" : "Isochore"; }

static bool loadHorizon( const DBKey& mid, od_ostream& strm )
{
    EM::Manager& em = EM::MGR();
    strm << "Loading horizon '" << em.objectName( mid ) << "'" << od_newline;
    LoggedTaskRunnerProvider trprov( strm );
    ConstRefMan<EM::Object> emobj = em.fetch( mid, trprov );
    if ( !emobj )
    {
	strm << "Failed to load horizon: ";
	strm << em.objectName( mid ).buf() << od_newline;
	return false;
    }

    emobj.setNoDelete( true );
    return true;
}


mLoad1Module("EarthModel")

bool BatchProgram::doWork( od_ostream& strm )
{
    strm << "Loading Horizons ..." << od_newline;
    DBKey mid1;
    pars().get( IsochronMaker::sKeyHorizonID(), mid1 );
    if ( !loadHorizon( mid1, strm ) )
	return false;

    EM::ObjectManager& mgr = EM::MGR();
    EM::Object* emobj1 = mgr.getObject( mid1 );
    mDynamicCastGet(EM::Horizon3D*,horizon1,emobj1);
    if ( !horizon1 )
	return false;

    DBKey mid2;
    pars().get( IsochronMaker::sKeyCalculateToHorID(), mid2 );
    if ( !loadHorizon( mid2, strm ) )
	return false;

    EM::Object* emobj2 = mgr.getObject( mid2 );
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

    strm << "Calculating " << getIsoMapType() << " ..." << od_newline;
    IsochronMaker maker( *horizon1, *horizon2, attrnm, dataidx );
    if ( SI().zIsTime() )
    {
	bool isinmsec = false;
	pars().getYN( IsochronMaker::sKeyOutputInMilliSecYN(), isinmsec );
	maker.setUnits( isinmsec );
    }

    if ( !maker.go( strm, false, false, 0 ) )
    {
	strm << "Failed to calculate " << getIsoMapType() << od_newline;
	horizon1->unRef(); horizon2->unRef();
	return false;
    }

    strm << getIsoMapType() << " '" << attrnm.buf() <<
					    "' calculated successfully\n";
    strm << "Saving " << getIsoMapType() << " Attribute ..." << od_newline;
    bool isoverwrite = false;
    pars().getYN( IsochronMaker::sKeyIsOverWriteYN(), isoverwrite );
    if ( !maker.saveAttribute( horizon1, dataidx, isoverwrite, &strm ) )
    {
	strm << "Failed to save " << getIsoMapType() << " Attribute" <<
							    od_newline;
	horizon1->unRef(); horizon2->unRef();
	return false;
    }

    strm << getIsoMapType() << " '" << attrnm.buf() << "' saved successfully"
							<< od_newline;
    horizon1->unRef(); horizon2->unRef();
    return true;
}

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


static ConstRefMan<EM::Horizon3D> loadHorizon( const MultiID& mid,
					       TaskRunner& runner )
{
    EM::EMManager& em = EM::EMM();
    PtrMan<Task> loader = em.objectLoader( mid );
    if ( !loader || !runner.execute(*loader.ptr()) )
	return nullptr;

    EM::ObjectID emid = em.getObjectID( mid );
    ConstRefMan<EM::EMObject> emobj = em.getObject( emid );
    if ( !emobj )
	return nullptr;

    ConstRefMan<EM::Horizon3D> horizon = dCast( const EM::Horizon3D*,
						emobj.ptr() );
    return horizon;
}


mLoad1Module("EarthModel")

bool BatchProgram::doWork( od_ostream& strm )
{
    strm << GetProjectVersionName() << od_newline;
    TextTaskRunner runner( strm );

    MultiID mid1;
    pars().get( IsochronMaker::sKeyHorizonID(), mid1 );
    ConstRefMan<EM::Horizon3D> horizon1 = loadHorizon( mid1, runner );
    if ( !horizon1 )
	return false;

    MultiID mid2;
    pars().get( IsochronMaker::sKeyCalculateToHorID(), mid2 );
    ConstRefMan<EM::Horizon3D> horizon2 = loadHorizon( mid2, runner );
    if ( !horizon2 )
	return false;

    BufferString attrnm;
    pars().get( IsochronMaker::sKeyAttribName(), attrnm );
    if ( attrnm.isEmpty() )
	return false;

    int dataidx = horizon1->auxdata.auxDataIndex( attrnm );
    if ( dataidx < 0 )
	dataidx = horizon1->auxdata.addAuxData( attrnm );

    IsochronMaker maker( *horizon1, *horizon2, attrnm, dataidx );
    if ( SI().zIsTime() )
    {
	bool isinmsec = false;
	pars().getYN( IsochronMaker::sKeyOutputInMilliSecYN(), isinmsec );
	maker.setUnits( isinmsec );
    }

    if ( !runner.execute(maker) )
    {
	strm << "Failed to calculate Isochron" << od_newline;
	return false;
    }

    strm << "Isochron '" << attrnm.buf() << "' calculated successfully\n";
    bool isoverwrite = false;
    pars().getYN( IsochronMaker::sKeyIsOverWriteYN(), isoverwrite );
    if ( !maker.saveAttribute(horizon1.ptr(),dataidx,
		   isoverwrite,&runner) )
    {
	strm << "Failed to save Isochron Attribute" << od_newline;
	return false;
    }

    strm << "Isochron '" << attrnm.buf() << "' saved successfully"
	 << od_newline;
    return true;
}

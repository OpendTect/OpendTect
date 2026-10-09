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
					       TaskRunner& runner )
{
    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    sdsel.rg = hs;
    PtrMan<Task> loader = em.objectLoader( mid, &sdsel );
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


mLoad3Modules("EMAttrib","WellAttrib","PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    strm << GetProjectVersionName() << od_newline;
    TextTaskRunner runner( strm );
    TrcKeySampling hs;
    if ( !getHorsampling( pars(), hs ) )
	return false;

    bool usesingle = false;
    pars().getYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle );
    MultiID mid1;
    pars().get( StratAmpCalc::sKeyTopHorizonID(), mid1 );
    ConstRefMan<EM::Horizon3D> tophor = loadHorizon( mid1, hs, runner );
    if ( !tophor )
	return false;

    ConstRefMan<EM::Horizon3D> bothor;
    if ( !usesingle )
    {
	MultiID mid2;
	pars().get( StratAmpCalc::sKeyBottomHorizonID(), mid2 );
	bothor = loadHorizon( mid2, hs, runner );
	if ( !bothor )
	    return false;
    }

    bool outputfold = false;
    pars().getYN( StratAmpCalc::sKeyOutputFoldYN(), outputfold );
    StratAmpCalc exec( tophor.ptr(), usesingle ? nullptr : bothor.ptr(),
		       hs, outputfold );
    if ( !exec.doInit(pars()) )
    {
	strm << "Cannot add attribute to Horizon" << od_newline;
	return false;
    }

    if ( !runner.execute(exec) )
	return false;

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
	if ( !exec.saveAttribute(addtotop ? *tophor : *bothor,attribidx,
				 isoverwrite,foldidx,&runner) )
	    return false;
    }

    strm << "Attribute saved successfully";
    return true;
}

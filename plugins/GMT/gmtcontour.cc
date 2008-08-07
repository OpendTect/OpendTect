/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: gmtcontour.cc,v 1.2 2008-08-07 12:10:18 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtcontour.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "filepath.h"
#include "initearthmodel.h"
#include "ioobj.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"



int GMTContour::factoryid_ = -1;

void GMTContour::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Contour", GMTContour::createInstance );
}

GMTPar* GMTContour::createInstance( const IOPar& iop )
{
    return new GMTContour( iop );
}


const char* GMTContour::userRef() const
{
    BufferString* str = new BufferString( "Contour: " );
    const char* nm = find( sKey::Name );
    *str += nm;
    return str->buf();
}


bool GMTContour::fillLegendPar( IOPar& par ) const
{
    BufferString str = find( sKey::Name );
    par.set( sKey::Name, str );
    bool drawcontour = false;
    getYN( ODGMT::sKeyDrawContour, drawcontour );
    if ( drawcontour )
    {
	str = find( ODGMT::sKeyLineStyle );
	par.set( ODGMT::sKeyLineStyle, str );
    }

    bool dofill = false;
    getYN( ODGMT::sKeyFill, dofill );
    if ( dofill )
    {
	par.set( ODGMT::sKeyPostColorBar, true );
	str = find( ODGMT::sKeyDataRange );
	par.set( ODGMT::sKeyDataRange, str );
    }

    return true;
}


bool GMTContour::execute( std::ostream& strm, const char* fnm )
{
    EarthModel::initStdClasses();
    MultiID id;
    get( sKey::ID, id );
    StepInterval<float> rg;
    get( ODGMT::sKeyDataRange, rg );
    bool closeps, drawcontour, dofill;
    getYN( ODGMT::sKeyClosePS, closeps );
    getYN( ODGMT::sKeyDrawContour, drawcontour );
    getYN( ODGMT::sKeyFill, dofill );

    const char* hornm = find( sKey::Name );
    strm << "Loading horizon " << hornm << " ...  ";
    strm.flush();
    EM::SurfaceIOData sd;
    PtrMan<IOPar> subpar = subselect( sKey::Selection );
    if ( !subpar )
	mErrStrmRet("Missing subselection")

    sd.rg.usePar( *subpar );
    PtrMan<EM::SurfaceIODataSelection> sel = new EM::SurfaceIODataSelection(sd);
    PtrMan<Executor> exec = EM::EMM().objectLoader( id, sel );
    if ( !exec || !exec->execute() )
	mErrStrmRet("Cannot load horizon")

    EM::ObjectID objid = EM::EMM().getObjectID( id );
    EM::EMObject* obj = EM::EMM().getObject( objid );
    if ( !obj )
	mErrStrmRet("Failed");

    strm << "Done" << std::endl;
    obj->ref();

    FilePath fp( fnm );
    fp.setExtension( "cpt" );
    BufferString cptfnm = fp.fullPath();
    strm << "Creating color pallette file ...  ";
    BufferString comm = "makecpt -Crainbow -T";
    comm += rg.start; comm += "/"; comm += rg.stop;
    comm += "/"; comm += rg.step;
    comm += " > "; comm += cptfnm;
    if ( system(comm) )
	mErrStrmRet("Failed")

    strm << "Done" << std::endl;
    strm << "Creating grid 100 X 100 ...  ";
    fp.setExtension( "gd1" );
    BufferString grd100fnm = fp.fullPath();
    comm = "@blockmean -R -I100 | surface -R -I100 -T0.5 -N250 -G";
    comm += grd100fnm;
    StreamData sdata = StreamProvider(comm).makeOStream();
    if ( !sdata.usable() ) mErrStrmRet("Failed")

    EM::SectionID sid = obj->sectionID( 0 );
    HorSamplingIterator iter( sd.rg );
    BinID bid;
    while ( iter.next(bid) )
    {
	EM::SubID subid = bid.getSerialized();
	Coord3 pos = obj->getPos( sid, subid );
	if ( !pos.isDefined() )
	    continue;

	*sdata.ostrm << pos.x << " " << pos.y << " " << pos.z << std::endl;
    }

    obj->unRef();
    sdata.close();
    strm << "Done" << std::endl;
    strm << "Regridding 25 X 25 ...  ";
    comm = "grdsample ";
    comm += grd100fnm; comm += " -I25 -G";
    fp.setExtension( "gd2" );
    comm += fp.fullPath();
    if ( system(comm) )
	mErrStrmRet("Failed")

    strm << "Done" << std::endl;
    if ( dofill )
    {
	strm << "Filling colors ...  ";
	comm = "grdimage "; comm += fp.fullPath();
	comm += " -R -J -O -C"; comm += cptfnm;
	if ( !closeps || drawcontour )
	    comm += " -K";

	comm += " >> "; comm += fnm;
	if ( system(comm) )
	    mErrStrmRet("Failed")

	strm << "Done" << std::endl;
    }

    if ( drawcontour )
    {
	strm << "Drawing contours ...  ";
	BufferString lskey = find( ODGMT::sKeyLineStyle );
	LineStyle ls; ls.fromString( lskey.buf() );
	BufferString lsstr;
	mGetLineStyleString( ls, lsstr );
	comm = "grdcontour "; comm += fp.fullPath();
	comm += " -R -J -O -C"; comm += cptfnm;
	comm += " -W"; comm += lsstr;
	if ( !closeps )
	    comm += " -K";

	comm += " >> "; comm += fnm;
	if ( system(comm) )
	    mErrStrmRet("Failed")

	strm << "Done" << std::endl;
    }

    strm << "Removing temporary grid files ...  ";
    StreamProvider( grd100fnm ).remove();
    StreamProvider( fp.fullPath() ).remove();
    strm << "Done" << std::endl;
    return true;
}




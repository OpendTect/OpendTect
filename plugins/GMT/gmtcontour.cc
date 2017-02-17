/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "gmtcontour.h"

#include "coltabsequence.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"
#include "pickset.h"
#include "strmprov.h"
#include "survinfo.h"

#include "od_ostream.h"



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
    const char* nm = find( sKey::Name() );
    *str += nm;
    return str->buf();
}


bool GMTContour::fillLegendPar( IOPar& par ) const
{
    par.set( sKey::Name(), find(sKey::Name()) );
    FixedString attrnm = find( ODGMT::sKeyAttribName() );
    BufferString str = "\""; str += attrnm;
    if ( attrnm == ODGMT::sKeyZVals() )
	str += SI().getZUnitString();

    str += "\"";
    par.set( ODGMT::sKeyAttribName(), str.buf() );
    bool drawcontour = false;
    getYN( ODGMT::sKeyDrawContour(), drawcontour );
    if ( drawcontour )
    {
	par.set( ODGMT::sKeyShape(), "Line" );
	par.set( sKey::Size(), 1 );
	str = find( ODGMT::sKeyLineStyle() );
	par.set( ODGMT::sKeyLineStyle(), str );
    }

    bool dofill = false;
    getYN( ODGMT::sKeyFill(), dofill );
    if ( dofill )
    {
	par.setYN( ODGMT::sKeyPostColorBar(), true );
	str = find( ODGMT::sKeyDataRange() );
	par.set( ODGMT::sKeyDataRange(), str );
    }

    return true;
}


bool GMTContour::execute( od_ostream& strm, const char* fnm )
{
    MultiID id;
    get( sKey::ID(), id );
    bool drawcontour=false, dofill=false;
    getYN( ODGMT::sKeyDrawContour(), drawcontour );
    getYN( ODGMT::sKeyFill(), dofill );

    const char* hornm = find( sKey::Name() );
    strm << "Loading horizon " << hornm << " ...  ";
    strm.flush();
    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !EM::EMM().getSurfaceData(id,sd,errmsg) )
	mErrStrmRet( errmsg.getFullString() )

    PtrMan<IOPar> subpar = subselect( sKey::Selection() );
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

    mDynamicCastGet( EM::Horizon3D*, hor, obj );
    if ( !hor )
	mErrStrmRet("Failed");

    strm << "Done" << od_endl;
    hor->ref();
    exec.erase();

    FixedString attribnm = find( ODGMT::sKeyAttribName() );
    const bool isz = attribnm == ODGMT::sKeyZVals();
    if ( !isz )
    {
	strm << "Loading Horizon Data \"" << attribnm << "\" ... ";
	const int selidx = sd.valnames.indexOf( attribnm.str() );
	exec = hor->auxdata.auxDataLoader( selidx );
	if ( !exec || !exec->execute() )
	    mErrStrmRet("Failed");

	exec.erase();
	strm << "Done" << od_endl;
    }

    FilePath fp( fnm );
    fp.setExtension( "cpt" );
    BufferString cptfnm( dofill ? fp.fullPath() : fp.getTempName("cpt") );

    strm << "Creating color pallette file ...  ";
    if ( !makeCPT(cptfnm.buf()) )
	mErrStrmRet("Failed")

    strm << "Done" << od_endl;
    strm << "Creating grid 100 X 100 ...  ";
    strm.flush();
    Coord spt1 = SI().transform( BinID(sd.rg.start_.inl(),sd.rg.start_.crl()) );
    Coord spt2 = SI().transform( BinID(sd.rg.start_.inl(),sd.rg.stop_.crl()) );
    Coord spt3 = SI().transform( BinID(sd.rg.stop_.inl(),sd.rg.start_.crl()) );
    Coord spt4 = SI().transform( BinID(sd.rg.stop_.inl(),sd.rg.stop_.crl()) );
    Coord botleft( mMIN( mMIN( spt1.x, spt2.x ), mMIN( spt3.x, spt4.x ) ),
		   mMIN( mMIN( spt1.y, spt2.y ), mMIN( spt3.y, spt4.y ) ) );
    Coord topright( mMAX( mMAX( spt1.x, spt2.x ), mMAX( spt3.x, spt4.x ) ),
		    mMAX( mMAX( spt1.y, spt2.y ), mMAX( spt3.y, spt4.y ) ) );
    fp.setExtension( "gd1" );
    BufferString grd100fnm = fileName( fp.fullPath() );
    BufferString rstr = "-R";
    rstr += botleft.x; rstr += "/"; rstr += topright.x; rstr += "/";
    rstr += botleft.y; rstr += "/"; rstr += topright.y;
    BufferString comm = "@blockmean "; comm += rstr;
    comm += " -I100 | surface "; comm += rstr; comm += " -I100 -T0.7 -N250 -G";
    comm += grd100fnm;
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed")

    TrcKeySamplingIterator iter( sd.rg );
    BinID bid;
    EM::SectionID sid = hor->sectionID( 0 );
    const float fac = mCast( float, SI().zDomain().userFactor() );
    const int dataidx = isz ? -1 : hor->auxdata.auxDataIndex( attribnm.str() );
    while ( iter.next(bid) )
    {
	EM::PosID posid( hor->id(), sid, bid.toInt64() );
	Coord3 pos = hor->getPos( posid );
	if ( !pos.isDefined() )
	    continue;

	const float val = isz ? (float) pos.z * fac
			      : hor->auxdata.getAuxDataVal( dataidx, posid );
	if ( mIsUdf(val) ) continue;

	procstrm << pos.x << " " << pos.y << " " << val << "\n";
    }

    procstrm.close();
    hor->unRef();
    strm << "Done" << od_endl;
    strm << "Regridding 25 X 25 ...  ";
    comm = "grdsample ";
    comm += grd100fnm; comm += " -I25 -G";
    fp.setExtension( "gd2" );
    comm += fileName( fp.fullPath() );
    if ( !execCmd(comm,strm) )
	mErrStrmRet("Failed")

    strm << "Done" << od_endl;

    Pick::Set ps;
    BufferString finalgrd = fileName( fp.fullPath() );
    BufferString mapprojstr;
    mGetRangeProjString( mapprojstr, "X" );
    if ( dofill )
    {
	strm << "Filling colors ...  ";
	comm = "grdimage "; comm += finalgrd;
	comm += " "; comm += mapprojstr;
	comm += " -O -Q -C"; comm += fileName( cptfnm );
	comm += " -K 1>> "; comm += fileName( fnm );
	if ( !execCmd(comm,strm) )
	    mErrStrmRet("Failed")

	strm << "Done" << od_endl;
    }

    if ( drawcontour )
    {
	strm << "Drawing contours ...  ";
	FixedString lskey = find( ODGMT::sKeyLineStyle() );
	LineStyle ls; ls.fromString( lskey.str() );
	BufferString lsstr;
	mGetLineStyleString( ls, lsstr );
	comm = "grdcontour "; comm += finalgrd;
	comm += " "; comm += mapprojstr;
	comm += " -O -C"; comm += fileName( cptfnm );
	BufferString colstr; mGetColorString( ls.color_, colstr );
	comm += " -A+f12p,Sans,"; comm += colstr;
	comm += " -W"; comm += lsstr;
	comm += " -K 1>> "; comm += fileName( fnm );
	if ( !execCmd(comm,strm) )
	    mErrStrmRet("Failed")

	strm << "Done" << od_endl;
    }

    strm << "Removing temporary grid files ...  ";
    StreamProvider( grd100fnm ).remove();
    StreamProvider( finalgrd ).remove();
    if ( !dofill )
	StreamProvider( cptfnm ).remove();

    strm << "Done" << od_endl;
    return true;
}


#define mPrintCol( col, endchar ) \
    procstrm << (int)col.r() << "\t"; \
    procstrm << (int)col.g() << "\t"; \
    procstrm << (int)col.b() << endchar;

bool GMTContour::makeCPT( const char* cptfnm ) const
{
    StepInterval<float> rg;
    get( ODGMT::sKeyDataRange(), rg );
    const char* seqname = find( ODGMT::sKeyColSeq() );
    if ( !seqname || !*seqname ) return false;

    ColTab::Sequence seq;
    if ( !ColTab::SM().get(seqname,seq) ) return false;

    bool doflip = false;
    getYN( ODGMT::sKeyFlipColTab(), doflip );
    od_ostream procstrm( cptfnm );
    if ( !procstrm.isOK() ) return false;

    procstrm << "#COLOR_MODEL = RGB" << "\n";
    const int nrsteps = rg.nrSteps();
    for ( int idx=0; idx<=nrsteps; idx++ )
    {
	const float val = rg.start + rg.step * idx;
	const float frac = (float)idx / (float)nrsteps;
	const Color col = seq.color( doflip ? 1 - frac : frac );
	if ( idx )
	{
	    procstrm << val << "\t";
	    mPrintCol( col, "\n" );
	}

	if ( idx < nrsteps )
	{
	    procstrm << val << "\t";
	    mPrintCol( col, "\t" );
	}
    }

    const Color bgcol = seq.color( mCast(float,doflip ? 1 : 0) );
    const Color fgcol = seq.color( mCast(float,doflip ? 0 : 1) );
    procstrm << "B" << "\t";  mPrintCol( bgcol, "\n" );
    procstrm << "F" << "\t";  mPrintCol( fgcol, "\n" );
    return true;
}

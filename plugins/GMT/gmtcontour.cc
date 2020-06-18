/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/

#include "gmtcontour.h"

#include "coltabseqmgr.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "initgmtplugin.h"
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

GMTPar* GMTContour::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTContour( iop, workdir );
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
	str += toString( SI().zUnitString() );

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


bool GMTContour::doExecute( od_ostream& strm, const char* fnm )
{
    DBKey id;
    get( sKey::ID(), id );
    bool drawcontour=false, dofill=false;
    getYN( ODGMT::sKeyDrawContour(), drawcontour );
    getYN( ODGMT::sKeyFill(), dofill );

    const char* hornm = find( sKey::Name() );
    strm << "Loading horizon " << hornm << " ...  ";
    strm.flush();
    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !EM::MGR().getSurfaceData(id,sd,errmsg) )
	mErrStrmRet( toString(errmsg) )

    PtrMan<IOPar> subpar = subselect( sKey::Selection() );
    if ( !subpar )
	mErrStrmRet("Missing subselection")

    sd.rg.usePar( *subpar );
    SilentTaskRunnerProvider trprov;
    ConstRefMan<EM::Object> obj = EM::Hor3DMan().fetch( id, trprov );
    if ( !obj )
	mErrStrmRet("Failed");

    mDynamicCastGet( const EM::Horizon3D*, hor, obj.ptr() );
    if ( !hor )
	mErrStrmRet("Failed");

    strm << "Done" << od_endl;
    hor->ref();

    FixedString attribnm = find( ODGMT::sKeyAttribName() );
    const bool isz = attribnm == ODGMT::sKeyZVals();
    if ( !isz )
    {
	strm << "Loading Horizon Data \"" << attribnm << "\" ... ";
	const int selidx = sd.valnames.indexOf( attribnm.str() );
	PtrMan<Executor> exec = hor->auxdata.auxDataLoader( selidx );
	if ( !exec || !exec->execute() )
	    mErrStrmRet("Failed");

	strm << "Done" << od_endl;
    }

    File::Path fp( fnm );
    fp.setPath( getWorkDir() );
    File::Path cptfp;
    BufferString cptfnm;
    if ( dofill )
    {
	cptfp = fp;
	cptfp.setExtension( "cpt" );
	cptfnm.set( cptfp.fileName() );
	cptfnm.clean( BufferString::AllowDots );
	cptfp.setFileName( cptfnm );
    }
    else
    {
	cptfp.setFileName( File::Path::getTempFileName("gmtcontour","cpt")  );
	cptfnm.set( cptfp.fileName() );
    }

    strm << "Creating color pallette file ...  ";
    if ( !makeCPT(cptfp.fullPath()) )
	mErrStrmRet("Failed")

    strm << "Done" << od_endl;
    strm << "Creating grid 100 X 100 ...  ";
    strm.flush();
    Coord spt1 = SI().transform( BinID(sd.rg.start_.inl(),sd.rg.start_.crl()) );
    Coord spt2 = SI().transform( BinID(sd.rg.start_.inl(),sd.rg.stop_.crl()) );
    Coord spt3 = SI().transform( BinID(sd.rg.stop_.inl(),sd.rg.start_.crl()) );
    Coord spt4 = SI().transform( BinID(sd.rg.stop_.inl(),sd.rg.stop_.crl()) );
    Coord botleft( mMIN( mMIN(spt1.x_,spt2.x_), mMIN(spt3.x_,spt4.x_) ),
		   mMIN( mMIN(spt1.y_,spt2.y_), mMIN(spt3.y_,spt4.y_) ) );
    Coord topright( mMAX( mMAX(spt1.x_,spt2.x_), mMAX(spt3.x_,spt4.x_) ),
		    mMAX( mMAX(spt1.y_,spt2.y_), mMAX(spt3.y_,spt4.y_) ) );
    fp.setExtension( "gd1" );
    BufferString grd100fnm( fp.fileName() );
    grd100fnm.clean( BufferString::AllowDots );
    BufferString rstr = "-R";
    rstr += botleft.x_; rstr += "/"; rstr += topright.x_; rstr += "/";
    rstr += botleft.y_; rstr += "/"; rstr += topright.y_;

    OS::MachineCommand blkmc( "blockmean" );
    blkmc.addArg( rstr ).addArg( "-I100" ).addPipe()
         .addArg( GMT::sKeyDefaultExec() ).addArg( "surface" )
	 .addArg( rstr ).addArg( "-I100" ).addArg( "-T0.7" )
         .addArg( "-N250" )
	 .addArg( BufferString("-G",grd100fnm) );

    od_ostream procstrm = makeOStream( blkmc, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed")

    TrcKeySamplingIterator iter( sd.rg );
    const float fac = mCast( float, SI().zDomain().userFactor() );
    const int dataidx = isz ? -1 : hor->auxdata.auxDataIndex( attribnm.str() );
    do
    {
	const EM::PosID posid = EM::PosID::getFromRowCol( iter.curBinID() );
	const Coord3 pos( hor->getPos( posid ) );
	if ( !pos.isDefined() )
	    continue;

	const float val = isz ? (float) pos.z_ * fac
			      : hor->auxdata.getAuxDataVal( dataidx, posid );
	if ( mIsUdf(val) ) continue;

	procstrm << pos.x_ << " " << pos.y_ << " " << val << "\n";
    } while ( iter.next() );

    procstrm.close();
    hor->unRef();
    strm << "Done" << od_endl;
    strm << "Regridding 25 X 25 ...  ";

    fp.setExtension( "gd2" );
    BufferString grd25fnm( fp.fileName() );
    grd25fnm.clean( BufferString::AllowDots );

    OS::MachineCommand grdsampmc( "grdsample" );
    grdsampmc.addArg( grd100fnm ).addArg( "-I25" )
	     .addArg( BufferString("-G",grd25fnm) );
    if ( !execCmd(grdsampmc,strm) )
	mErrStrmRet("Failed")

    strm << "Done" << od_endl;

    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")
    if ( dofill )
    {
	strm << "Filling colors ...  ";
	OS::MachineCommand grdmc( "grdimage" );
	grdmc.addArg( grd25fnm )
	     .addArg( mapprojstr ).addArg( rgstr ).addArg( "-O" ).addArg( "-K" )
	     .addArg( BufferString("-C",cptfnm) ).addArg( "-Q" );
	if ( !execCmd(grdmc,strm,fnm) )
	    mErrStrmRet("Failed")

	strm << "Done" << od_endl;
    }

    if ( drawcontour )
    {
	strm << "Drawing contours ...  ";
	FixedString lskey = find( ODGMT::sKeyLineStyle() );
	OD::LineStyle ls; ls.fromString( lskey.str() );
	BufferString lsstr;
	mGetLineStyleString( ls, lsstr );
	BufferString colstr; mGetColorString( ls.color_, colstr );
	OS::MachineCommand grdcontmc( "grdcontour" );
	grdcontmc.addArg( grd25fnm )
	         .addArg( mapprojstr ).addArg( rgstr )
		 .addArg( "-O" ).addArg( "-K" )
	         .addArg( BufferString("-C",cptfnm) )
		 .addArg( BufferString("-A+f12p,Helvetica,",colstr) )
		 .addArg( lsstr.insertAt(0,"-W") );
	if ( !execCmd(grdcontmc,strm,fnm) )
	    mErrStrmRet("Failed")

	strm << "Done" << od_endl;
    }

    strm << "Removing temporary grid files ...  ";
    File::remove( File::Path(fp.pathOnly(),grd100fnm).fullPath() );
    File::remove( File::Path(fp.pathOnly(),grd25fnm).fullPath() );
    if ( !dofill )
	File::remove( cptfnm );

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

    ConstRefMan<ColTab::Sequence> seq = ColTab::SeqMGR().getAny( seqname );

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
	const Color col = seq->color( doflip ? 1 - frac : frac );
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

    const Color bgcol = seq->color( mCast(float,doflip ? 1 : 0) );
    const Color fgcol = seq->color( mCast(float,doflip ? 0 : 1) );
    procstrm << "B" << "\t";  mPrintCol( bgcol, "\n" );
    procstrm << "F" << "\t";  mPrintCol( fgcol, "\n" );
    return true;
}

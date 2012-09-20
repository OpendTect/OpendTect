/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "prog.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "initearthmodel.h"
#include "ioman.h"
#include "posinfo.h"
#include "position.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "strmprov.h"
#include "survinfo.h"
#include "rcol.h"
#include "executor.h"

#define NaN 9

/*  The Input file should be in the following format:
    LineName	Trace	Hor1_Z	Hor2_Z	.................

    Then the command line should read something like this:
    ./import_hor2d [Input File] [LineSet] [Hor1] [Hor2] ..............
    
    Struct HorLine2D contains data for all Horizons for a line. It basically 
    represents a 2D Line from the input file.
*/

struct HorLine2D
{
    TypeSet<Coord>		pos_;
    TypeSet<int>		traces_;
    TypeSet< TypeSet<float> >	zvals_;

    BufferString		linename_;
};


static int prUsage( const char* msg = 0 )
{
    std::cerr << "Usage: InputAsciiFile LineSetName HorizonName(s)";
    if ( msg ) std::cerr << '\n' << msg;
    std::cerr << std::endl;
    return 1;
}


static int prError( const char* msg )
{
    std::cerr << msg << std::endl;
    return 0;
}


HorLine2D* newLine( const char* linename )
{
    HorLine2D* newline = new HorLine2D;
    newline->linename_ = linename;
    return newline;
}


bool getPos( const PosInfo::Line2DData& line, int trcnr, Coord& xypos )
{
    int previdx = -1;
    int curtr = -1;
    for ( int idx=0; idx<line.posns.size(); idx++ )
    {
	curtr = line.posns[idx].nr_;
	if ( curtr == trcnr )
	{
	    xypos = line.posns[idx].coord_;
	    return true;
	}
	else if ( curtr > trcnr ) break;

	previdx = idx;
    }

    if ( previdx<0 || previdx >= line.posns.size()-1 )
	return false;

    const int prevtr = line.posns[previdx].nr_;
    const Coord prevpos = line.posns[previdx].coord_;
    const Coord nextpos = line.posns[previdx+1].coord_;
    const float factor = ( trcnr - prevtr ) / ( curtr - prevtr );
    if ( factor<0 || factor>1 ) return false;

    xypos = prevpos + (nextpos-prevpos) * factor;
    return true;
}


bool readFromFile( ObjectSet<HorLine2D>& data, const char* filename,
  		   const char* linesetnm, const int nrhors )
{
    StreamProvider sp( filename );
    StreamData sd = sp.makeIStream();
    std::cerr << "Input File= " << filename << "\n";
    if ( !sd.usable() ) return prError( "input file is not OK" );

    char buf[1024]; char valbuf[80];
    HorLine2D* linedata = 0;

    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    PtrMan<IOObj> lsetobj = IOM().getLocal( linesetnm );
    BufferString msg( "Cannot find LineSet: " ); msg += linesetnm; 
    if ( !lsetobj ) return prError( msg );
    BufferString fnm = lsetobj->fullUserExpr(true);
    Seis2DLineSet lineset( fnm );

    PosInfo::Line2DData line2d;
    while ( sd.istrm->good() )
    {
	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	if ( !ptr || !*ptr )
	    continue;

	if ( !linedata )
	{
	    linedata = newLine( valbuf );
	    
	    removeTrailingBlanks( valbuf );
	    int lineidx = lineset.indexOf( valbuf );
	    if ( !linedata || !lineset.getGeometry(lineidx,line2d) ) break;
	}

	if ( linedata && strcmp(valbuf,linedata->linename_) )
	{ 
	    data += linedata;
	    linedata = newLine( valbuf );
	    int lineidx = lineset.indexOf( valbuf );
	    if ( !linedata || !lineset.getGeometry(lineidx,line2d) ) break;
	}
	
	ptr = getNextWord( ptr, valbuf );
	const int trcnr = toInt( valbuf );
	Coord xypos;
	if( !getPos(line2d,trcnr,xypos) )
	{
	    std::cerr << "Skipping Trace No. " << trcnr << "\n";
	    continue;
	}

	linedata->traces_ += trcnr;
	linedata->pos_ += xypos;

	ptr = getNextWord( ptr, valbuf );
	TypeSet<float> vals;
	for ( int hdx=0; hdx<nrhors; hdx++ )
	{
	    float val = 0;
	    if ( valbuf )
		val = toFloat( valbuf );
	    if ( !mIsUdf(val) )
		vals += SI().zIsTime() ? val/1000 : val;
	    else
		vals += mUdf(float);
	    ptr = getNextWord( ptr, valbuf );
	}
	linedata->zvals_ += vals;
    }

    sd.close();
    if ( linedata )
	data += linedata;

    return data.size();
}

int addLine( EM::Horizon2D* hor, HorLine2D* horline, const MultiID& lsetkey,
						     TypeSet<Coord>& pos )
{
    pos += horline->pos_[0];
    for ( int tdx=1; tdx<horline->traces_.size(); tdx++ )
    {
	const int trc1 = horline->traces_[tdx-1];
	const int trc2 = horline->traces_[tdx];
	const int trcintv = trc2 - trc1;
	const float x1 = horline->pos_[tdx-1].x;
	const float x2 = horline->pos_[tdx].x;
	const float xdifpertrc = (x2 - x1) / trcintv;
	const float y1 = horline->pos_[tdx-1].y;
	const float y2 = horline->pos_[tdx].y;
	const float ydifpertrc = (y2 - y1) / trcintv;

	for ( int idx=trc1+1; idx<=trc2; idx++ )
	{
	    const float x = x1 + xdifpertrc * (idx - trc1);
	    const float y = y1 + ydifpertrc * (idx - trc1);
	    pos += Coord( x, y );
	}
    }

    int lineid = hor->geometry().addLine( pos, horline->traces_[0], 1,
					lsetkey, horline->linename_ );
    return lineid;
}


void makeHorizons( ObjectSet<HorLine2D>& data, const MultiID& lsetkey,
		   ObjectSet<EM::Horizon2D>& horizons )
{
    for ( int ldx=0; ldx<data.size(); ldx++ )
    {
	int start = data[ldx]->traces_[0];
	int step = 1;
	std::cerr << "Processing Line " << data[ldx]->linename_ << "\n";
	for ( int hdx=0; hdx<horizons.size(); hdx++ )
	{
	    TypeSet<Coord> pos;
	    int lineid = addLine( horizons[hdx], data[ldx], lsetkey, pos );
	    int tdx = 0;
	    for ( ; tdx<data[ldx]->traces_.size(); tdx++ )
		if ( data[ldx]->zvals_[tdx][hdx] < NaN )
		    break;
	    if ( tdx >= data[ldx]->traces_.size() )
		continue;

	    RowCol rowcol( lineid, data[ldx]->traces_[tdx] );
	    EM::SubID subid = rowcol.getSerialized();
	    Coord3 xypos1( pos[0], data[ldx]->zvals_[tdx][hdx] );
	    horizons[hdx]->setPos( horizons[hdx]->sectionID(0), subid,
				xypos1, false );
	    int tdx1 = tdx++;
	    int tdx2 = tdx1;
	    while ( true )
	    {
		tdx1 = tdx2++;
		const float val1 = data[ldx]->zvals_[tdx1][hdx];
		while ( tdx2<data[ldx]->traces_.size() && 
			data[ldx]->zvals_[tdx2][hdx] >= NaN )
		    tdx2++;
		if ( tdx2>=data[ldx]->traces_.size() )
		    break;
		const float val2 = data[ldx]->zvals_[tdx2][hdx];
		
		const int trc1 = data[ldx]->traces_[tdx1];
		const int trc2 = data[ldx]->traces_[tdx2];
		const int trcintv = trc2 - trc1;
		
		const float valdifpertrc = (val2 - val1) / trcintv;
		for ( int idx=trc1+1; idx<=trc2; idx++ )
		{
		    const float val = val1 + valdifpertrc * (idx - trc1);
		    const int trcid = idx - data[ldx]->traces_[0];
		    RowCol rowcol( lineid, idx );
		    EM::SubID subid = rowcol.getSerialized();
		    Coord3 xypos( pos[trcid], val );
		    horizons[hdx]->setPos( horizons[hdx]->sectionID(0), subid,
				xypos, false );
		}
	    }
	}
    }
}


static int doWork( int argc, char** argv )
{
    if ( argc < 4 ) return prUsage();
    BufferString errmsg;

    ObjectSet<HorLine2D> data;
    if ( !readFromFile( data, argv[1], argv[2], argc-3 ) )
	return 1;

    EarthModel::initStdClasses();
    ObjectSet<EM::Horizon2D> horizons;
    for ( int hdx=0; hdx<argc-3; hdx++ )
    {
	const char* horizonnm = argv[hdx+3];
	EM::EMManager& em = EM::EMM();
	EM::ObjectID horid = em.createObject( EM::Horizon2D::typeStr(),
					horizonnm );
	mDynamicCastGet(EM::Horizon2D*,hor,em.getObject(horid));
	if ( !hor )
	{
	    prError( "Cannot Create Horizon\n" );
	    break;
	}

	hor->ref();
	horizons += hor;
    }

    if ( !horizons.size() ) return 1;

    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    PtrMan<IOObj> lsetobj = IOM().getLocal( argv[2] );
    const MultiID lsetkey = lsetobj->key();
    makeHorizons( data, lsetkey, horizons );

    std::cerr << "Saving data ..." << std::endl;
    for ( int hdx=0; hdx<horizons.size(); hdx++ )
    {
	PtrMan<Executor> saver = horizons[hdx]->saver();
    	saver->execute();
	horizons[hdx]->unRef();
    }
    
    deepErase( data );

    return 0;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}

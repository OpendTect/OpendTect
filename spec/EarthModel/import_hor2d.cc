/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          June 2007
 RCS:           $Id: import_hor2d.cc,v 1.1 2007-06-15 05:16:40 cvsraman Exp $
________________________________________________________________________

-*/

#include "prog.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "position.h"
#include "strmprov.h"
#include "survinfo.h"
#include "rcol.h"
#include "executor.h"

#define NaN 9

/*  The Input file should be in the following format:
    LineName	Trace	X	Y	Hor1_Z	Hor2_Z	.................

    Then the command line should read something like this:
    ./import_hor2d [Input File] [Hor1] [Hor2] ..............
    
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
    std::cerr << "Usage: [Input file] [Horizon Name(s)]";
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


bool readFromFile( ObjectSet<HorLine2D>& data, const char* filename,
       					       const int nrhors	)
{
    StreamProvider sp( filename );
    std::cerr << sp.fullName()<<"\n"<<sp.fileName()<<"\n"<<sp.hostName()<<"\n";
    StreamData sd = sp.makeIStream();
    std::cerr << "Input File= " << filename << "\n";
    if ( !sd.usable() ) return prError( "input file is not OK" );

    char buf[1024]; char valbuf[80];
    HorLine2D* linedata = 0;

    while ( sd.istrm->good() )
    {
	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	if ( !ptr || !*ptr )
	    continue;

	if ( !linedata )
	    linedata = newLine( valbuf );

	if ( linedata && strcmp(valbuf,linedata->linename_) )
	{ 
	    data += linedata;
	    linedata = newLine( valbuf );
	    if ( !linedata ) break;
	}
	
	ptr = getNextWord( ptr, valbuf );
	linedata->traces_ += atoi( valbuf );
	ptr = getNextWord( ptr, valbuf );

	Coord xypos;
	xypos.x = atof( valbuf );
	ptr = getNextWord( ptr, valbuf );

	xypos.y = atof( valbuf );
	linedata->pos_ += xypos;
	ptr = getNextWord( ptr, valbuf );
	TypeSet<float> vals;
	for ( int hdx=0; hdx<nrhors; hdx++ )
	{
	    float val = 0;
	    if ( valbuf )
		val = atof( valbuf );
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

int addLine( EM::Horizon2D* hor, HorLine2D* horline, const char* lineset,
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
					lineset, horline->linename_ );
    return lineid;
}


void makeHorizons( ObjectSet<HorLine2D>& data, 
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
	    int lineid = addLine( horizons[hdx], data[ldx], "Lineset1", pos );
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
    if ( argc < 3 ) return prUsage();
    BufferString errmsg;

    ObjectSet<HorLine2D> data;
    if ( !readFromFile( data, argv[1], argc-2 ) )
	return 1;

    ObjectSet<EM::Horizon2D> horizons;
    for ( int hdx=0; hdx<argc-2; hdx++ )
    {
	const char* horizonnm = argv[hdx+2];
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

    makeHorizons( data, horizons );

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

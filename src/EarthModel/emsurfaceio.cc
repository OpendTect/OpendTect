/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emsurfaceio.cc,v 1.35 2004-06-07 11:59:49 kristofer Exp $";

#include "emsurfaceio.h"

#include "color.h"
#include "ascstream.h"
#include "datachar.h"
#include "datainterp.h"
#include "emsurface.h"
#include "emhingeline.h"
#include "emsurfauxdataio.h"
#include "filegen.h"
#include "geommeshsurface.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "streamconn.h"


const char* EM::dgbSurfaceReader::floatdatacharstr = "Data char";
const char* EM::dgbSurfaceReader::nrhingelinestr = "Nr hingelines";
const char* EM::dgbSurfaceReader::hingelineprefixstr = "HingeLine ";

const char* EM::dgbSurfaceReader::nrposattrstr = "Nr Pos Attribs";
const char* EM::dgbSurfaceReader::posattrprefixstr = "Pos Attrib ";
const char* EM::dgbSurfaceReader::posattrpatchstr = " Patch";
const char* EM::dgbSurfaceReader::posattrposidstr = " SubID";

const char* EM::dgbSurfaceReader::intdatacharstr = "Int Data char";
const char* EM::dgbSurfaceReader::nrpatchstr = "Nr Patches";
const char* EM::dgbSurfaceReader::patchidstr = "Patch ";
const char* EM::dgbSurfaceReader::patchnamestr = "Patch Name ";
const char* EM::dgbSurfaceReader::rowrangestr = "Row range";
const char* EM::dgbSurfaceReader::colrangestr = "Col range";
const char* EM::dgbSurfaceReader::dbinfostr = "DB info";
const char* EM::dgbSurfaceReader::versionstr = "Format version";
const char* EM::dgbSurfaceReader::prefcolofstr = "Color";

const char* EM::dgbSurfaceReader::badconnstr = "Internal: Bad connection";
const char* EM::dgbSurfaceReader::parseerrorstr = "Cannot parse file";


EM::dgbSurfaceReader::dgbSurfaceReader( const IOObj& ioobj,
					const char* filetype,
					EM::Surface* surface_)
    : ExecutorGroup( "Surface Reader" )
    , conn( dynamic_cast<StreamConn*>(ioobj.getConn(Conn::Read)) )
    , surface( surface_ )
    , par( 0 )
    , readrowrange( 0 )
    , readcolrange( 0 )
    , intinterpreter( 0 )
    , floatinterpreter( 0 )
    , nrdone( 0 )
    , patchindex( 0 )
    , oldpatchindex( -1 )
    , surfposcalc( 0 )
    , rcconv( 0 )
    , readfilltype( false )
{
    auxdataexecs.allowNull(true);
    if ( !conn || !conn->forRead()  )
    {
	msg = "Cannot open input surface file";
	error = true;
	return;
    }

    std::istream& strm = ((StreamConn*)conn)->iStream();
    ascistream astream( strm );
    if ( !astream.isOfFileType( filetype ))
    {
	msg = "Invalid filetype";
	error = true;
	return;
    }

    astream.next();

    par = new IOPar( astream, false );
    int nrpatches;
    if ( !par->get( nrpatchstr, nrpatches ) &&
	    !par->get("Nr Subhorizons", nrpatches ) )
    {
	msg = parseerrorstr;
	error = true;
	return;
    }

    for ( int idx=0; idx<nrpatches; idx++ )
    {
	int patchid = idx;
	BufferString key = patchidstr;
	key += idx;
	par->get(key, patchid);
	patchids+= patchid;

	key = patchnamestr;
	key += idx;
	BufferString patchname;
	par->get(key,patchname);
	if ( !patchname.size() )
	{ patchname = "["; patchname += idx+1; patchname += "]"; }
		
	patchnames += new BufferString(patchname);
    }

    par->get( rowrangestr, rowrange.start, rowrange.stop, rowrange.step );
    par->get( colrangestr, colrange.start, colrange.stop, colrange.step );

    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 50 ) break;

	BufferString hovfnm( 
		EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx) );
	if ( File_isEmpty(hovfnm) )
	{ gap++; continue; }

	EM::dgbSurfDataReader* dreader = new EM::dgbSurfDataReader( hovfnm );
	if ( dreader->dataName() )
	{
	    auxdatanames += new BufferString(dreader->dataName());
	    auxdataexecs += dreader;
	}
	else
	{
	    delete dreader;
	    break;
	}
    }

    BufferString dc;
    if ( par->get(intdatacharstr,dc) )
    {
	DataCharacteristics writtendatachar;
	writtendatachar.set( dc.buf() );
	intinterpreter = new DataInterpreter<int>( writtendatachar );

	if ( !par->get(floatdatacharstr,dc) )
	{
	    msg = parseerrorstr;
	    error = true;
	    return;
	}

	writtendatachar.set( dc.buf() );
	floatinterpreter = new DataInterpreter<double>( writtendatachar );
    }

    for ( int idx=0; idx<nrPatches(); idx++ )
	patchsel += patchID(idx);

    for ( int idx=0; idx<nrAuxVals(); idx++ )
	auxdatasel += idx;

    par->get( dbinfostr, dbinfo );


    error = false;
}


const char* EM::dgbSurfaceReader::dbInfo() const
{
    return surface ? surface->dbInfo() : "";
}


EM::dgbSurfaceReader::~dgbSurfaceReader()
{
    deepErase( patchnames );
    deepErase( auxdatanames );
    deepErase( auxdataexecs );

    delete par;
    delete conn;
    delete surfposcalc;
    delete rcconv;
    delete readrowrange;
    delete readcolrange;
}


bool EM::dgbSurfaceReader::isOK() const
{
    return !error;
}


int EM::dgbSurfaceReader::nrPatches() const
{
    return patchnames.size();
}


EM::PatchID EM::dgbSurfaceReader::patchID( int idx ) const
{
    return patchids[idx];
}


const char* EM::dgbSurfaceReader::patchName( int idx ) const
{
    const char* res = patchnames[idx]->buf();
    return res && *res ? res : 0;
}


void EM::dgbSurfaceReader::selPatches(const TypeSet<EM::PatchID>& sel)
{
    patchsel = sel;
}


int EM::dgbSurfaceReader::nrAuxVals() const
{
    return auxdatanames.size();
}


const char* EM::dgbSurfaceReader::auxDataName( int idx ) const
{
    return *auxdatanames[idx];
}


void EM::dgbSurfaceReader::selAuxData(const TypeSet<int>& sel )
{
    auxdatasel = sel;
}


const StepInterval<int>& EM::dgbSurfaceReader::rowInterval() const
{
    return rowrange;
}


const StepInterval<int>& EM::dgbSurfaceReader::colInterval() const
{
    return colrange;
}


void EM::dgbSurfaceReader::setRowInterval( const StepInterval<int>& rg )
{
    if ( readrowrange ) delete readrowrange;
    readrowrange = new StepInterval<int>(rg);
}


void EM::dgbSurfaceReader::setColInterval( const StepInterval<int>& rg )
{
    if ( readcolrange ) delete readcolrange;
    readcolrange = new StepInterval<int>(rg);
}


const IOPar* EM::dgbSurfaceReader::pars() const
{
    return par;
}


void EM::dgbSurfaceReader::setSurfPosCalc( SurfPosCalc* n )
{
    delete surfposcalc;
    surfposcalc = n;
}


void EM::dgbSurfaceReader::setRowColConverter( RowColConverter* n )
{
    delete rcconv;
    rcconv = n;
}


void EM::dgbSurfaceReader::setReadFillType( bool yn )
{
    readfilltype = yn;
}


int EM::dgbSurfaceReader::nrDone() const
{
    return (executors.size() ? ExecutorGroup::nrDone() : 0) + nrdone;
}


const char* EM::dgbSurfaceReader::nrDoneText() const
{
    return "Gridlines read";
}


int EM::dgbSurfaceReader::totalNr() const
{
    int ownres =
	(readrowrange?readrowrange->nrSteps():rowrange.nrSteps()) *
	patchids.size();

    if ( !ownres ) ownres = nrrows;

    return ownres + (executors.size() ? ExecutorGroup::totalNr() : 0);


}


int EM::dgbSurfaceReader::nextStep()
{
    if ( error || !surface )
    {
	if ( !surface ) 
	    msg = "Internal: No Surface Set";

	return ErrorOccurred;
    }

    if ( !nrdone )
    {
	surface->setDBInfo( dbinfo );
	for ( int idx=0; idx<auxdatasel.size(); idx++ )
	{
	    if ( auxdatasel[idx]>=auxdataexecs.size() )
		continue;

	    auxdataexecs[auxdatasel[idx]]->setSurface( *surface );

	    add( auxdataexecs[auxdatasel[idx]] );
	    auxdataexecs.replace( 0, auxdatasel[idx] );
	}

	for ( int idx=0; idx<patchsel.size(); idx++ )
	{
	    const int index = patchids.indexOf(patchsel[idx]);
	    if ( index<0 )
	    {
		patchsel.remove(idx--);
		continue;
	    }
	}

	if ( readrowrange )
	{
	    const RowCol filestep = rcconv 
	    		? rcconv->get(RowCol(1,1))-rcconv->get(RowCol(0,0))
			: RowCol(rowrange.step, colrange.step);

	    if ( readrowrange->step < abs(filestep.row) )
		readrowrange->step = filestep.row;
	    if ( readcolrange->step < abs(filestep.col) )
		readcolrange->step = filestep.col;

	    if ( readrowrange->step / filestep.row < 0 )
		readrowrange->step *= -1;
	    if ( readcolrange->step / filestep.col < 0 )
		readcolrange->step *= -1;
	}
    }

    if ( patchindex>=patchids.size() )
    {
	int col;
	if ( par->get( prefcolofstr, col ) )
	{
	    Color newcol; newcol.setRgb(col);
	    surface->setPreferredColor(newcol);
	}

	for ( int idx=0; idx<surface->nrHingeLines(); idx++ )
	    surface->removeHingeLine(idx,false);

	int nrhingelines = 0;
	par->get( nrhingelinestr, nrhingelines );
	for ( int idx=0; idx<nrhingelines; idx++ )
	{
	    BufferString key = hingelineprefixstr; key += idx;
	    PtrMan<IOPar> relpar = par->subselect(key);
	    if ( !relpar )
		return -1;

	    EM::HingeLine* hingeline = new EM::HingeLine(*surface);
	    if ( !hingeline->usePar( *relpar ) )
	    {
		delete hingeline;
		return -1;
	    }

	    if ( surface->hasPatch(hingeline->getSection()) )
		surface->addHingeLine( hingeline, false );
	    else
		delete hingeline;
	}


	for ( int idx=0; idx<surface->nrPosAttribs(); idx++ )
	    surface->removePosAttrib(surface->posAttrib(idx));

	int nrattribs = 0;
	par->get( nrposattrstr, nrattribs );
	for ( int idx=0; idx<nrattribs; idx++ )
	{
	    BufferString attribkey = EM::dgbSurfaceReader::posattrprefixstr;
	    attribkey += idx;

	    int attrib;
	    if ( !par->get( attribkey, attrib ) )
		continue;

	    TypeSet<int> attrpatches;
	    TypeSet<long long> subids;

	    BufferString patchkey = attribkey;
	    patchkey += EM::dgbSurfaceReader::posattrpatchstr;
	    BufferString subidkey = attribkey;
	    subidkey += EM::dgbSurfaceReader::posattrposidstr;

	    par->get( patchkey, attrpatches );
	    par->get( subidkey, subids );

	    for ( int idy=0; idy<attrpatches.size(); idy++ )
	    {
		const EM::PosID pid(surface->id(),attrpatches[idy],subids[idy]);
		surface->setPosAttrib( pid, attrib, true );
	    }
	}

	int res = ExecutorGroup::nextStep();
	if ( !res )
	    surface->resetChangedFlag();
	return res;
    }

    std::istream& strm = conn->iStream();

    static const char* readerrmsg = "Unexpected end of file";

    if ( patchindex!=oldpatchindex )
    {
	nrrows = readInt(strm);
	if ( !strm )
	{
	    msg = readerrmsg;
	    return ErrorOccurred;
	}

	if ( nrrows )
	    firstrow = readInt(strm);
	else
	{
	    patchindex++;
	    return MoreToDo;
	}

	rowindex = 0;


	oldpatchindex = patchindex;
    }

    const EM::PatchID patchid = patchids[patchindex];
    const int filerow = firstrow+rowindex*rowrange.step;

    const int nrcols = readInt(strm);
    if ( !strm )
    {
	msg = readerrmsg;
	return ErrorOccurred;
    }
    const int firstcol = nrcols ? readInt(strm) : 0;

    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
	const int filecol = firstcol+colindex*colrange.step;

	const RowCol rowcol = rcconv
	    ? rcconv->get(RowCol(filerow,filecol)) : RowCol(filerow,filecol);

	Coord3 pos;
	if ( !surfposcalc )
	{
	    pos.x = readFloat(strm);
	    pos.y = readFloat(strm);
	}
	else
	{
	    const Coord c = surfposcalc->getPos( rowcol );
	    pos.x = c.x;
	    pos.y = c.y;
	}

	pos.z = readFloat(strm);

	if ( readfilltype && rowindex!=nrrows-1 && colindex!=nrcols-1 )
	    readInt(strm);

	if ( patchsel.indexOf(patchid)==-1 )
	    continue;

	if ( readrowrange && (!readrowrange->includes(rowcol.row) ||
		    ((rowcol.row-readrowrange->start)%readrowrange->step)))
	    continue;

	if ( readcolrange && (!readcolrange->includes(rowcol.col) ||
		    ((rowcol.col-readcolrange->start)%readcolrange->step)))
	    continue;

	if ( !surface->getSurface(patchid) )
	{
	    const RowCol filestep = rcconv 
			    ? rcconv->get(RowCol(1,1))-rcconv->get(RowCol(0,0))
			    : RowCol(rowrange.step, colrange.step);

	    surface->setTranslatorData( filestep, 
		    readrowrange ? RowCol(readrowrange->step,readcolrange->step)
				 : filestep,
		    rowcol, readrowrange, readcolrange );

	    const int index = patchids.indexOf(patchid);
	    surface->addPatch( *patchnames[index], patchids[index], false );
	}

	surface->setPos( patchid, rowcol, pos, false, false );
    }

    rowindex++;
    if ( rowindex>=nrrows )
	patchindex++;

    nrdone++;

    return MoreToDo;
}


const char* EM::dgbSurfaceReader::message() const
{
    return msg;
}


int EM::dgbSurfaceReader::readInt(std::istream& strm) const
{
    if ( intinterpreter )
    {
	const int sz = intinterpreter->nrBytes();
	char buf[sz];
	strm.read(buf,sz);
	return intinterpreter->get(buf,0);
    }

    int res;
    strm >> res;
    return res;
}


double EM::dgbSurfaceReader::readFloat(std::istream& strm) const
{
    if ( floatinterpreter )
    {
	const int sz = floatinterpreter->nrBytes();
	char buf[sz];
	strm.read(buf,sz);
	return floatinterpreter->get(buf,0);
    }

    double res;
    strm >> res;
    return res;
}


EM::dgbSurfaceWriter::dgbSurfaceWriter( const IOObj* ioobj_,
					const char* filetype_,
					const EM::Surface& surface_,
					bool binary_)
    : ExecutorGroup( "Surface Writer" )
    , conn( 0 )
    , ioobj( ioobj_ ? ioobj_->clone() : 0 )
    , surface( surface_ )
    , par( *new IOPar("Surface parameters" ))
    , writerowrange( new StepInterval<int> )
    , writecolrange( new StepInterval<int> )
    , nrdone( 0 )
    , patchindex( 0 )
    , oldpatchindex( -1 )
    , writeonlyz( false )
    , filetype( filetype_ )
    , binary( binary_ )
{
    par.set( EM::dgbSurfaceReader::dbinfostr, surface.dbInfo() );

    if ( binary )
    {
	BufferString dc;
	int dummy;
	DataCharacteristics(dummy).toString( dc.buf() );
	par.set( EM::dgbSurfaceReader::intdatacharstr, dc );

	float fdummy;
	DataCharacteristics(fdummy).toString( dc.buf() );
	par.set( EM::dgbSurfaceReader::floatdatacharstr, dc );
    }

    for ( int idx=0; idx<nrPatches(); idx++ )
	patchsel += patchID(idx);

    for ( int idx=0; idx<nrAuxVals(); idx++ )
    {
	if ( auxDataName(idx) )
	    auxdatasel += idx;
    }

    par.set( EM::dgbSurfaceReader::prefcolofstr, (int) surface.preferredColor().rgb() );

    par.set( EM::dgbSurfaceReader::nrhingelinestr, surface.nrHingeLines() );
    int hingeid = 0;
    for ( int idx=0; idx<surface.nrHingeLines(); idx++ )
    {
	if ( !surface.hingeLine(idx) )
	    continue;

	BufferString key = EM::dgbSurfaceReader::hingelineprefixstr;
	key += hingeid++;
	IOPar relpar;
	surface.hingeLine(idx)->fillPar(relpar);
	par.mergeComp( relpar, key );
    }

    int keyid = 0;
    for ( int idx=0; idx<surface.nrPosAttribs(); idx++ )
    {
	const int attrib = surface.posAttrib(idx);
	const TypeSet<PosID>* pids = surface.getPosAttribList(attrib);
	if ( !pids ) continue;

	BufferString attribkey = EM::dgbSurfaceReader::posattrprefixstr;
	attribkey += keyid++;
	par.set( attribkey, attrib );

	TypeSet<int> attrpatches;
	TypeSet<long long> subids;
	for ( int idy=0; idy<pids->size(); idy++ )
	{
	    attrpatches += (*pids)[idy].patchID();
	    subids += (*pids)[idy].subID();
	}

	BufferString patchkey = attribkey;
	patchkey += EM::dgbSurfaceReader::posattrpatchstr;
	BufferString subidkey = attribkey;
	subidkey += EM::dgbSurfaceReader::posattrposidstr;

	par.set( patchkey, attrpatches );
	par.set( subidkey, subids );
    }

    par.set( EM::dgbSurfaceReader::nrposattrstr, keyid );
    surface.getRange( *writerowrange, true );
    surface.getRange( *writecolrange, false );
}


EM::dgbSurfaceWriter::~dgbSurfaceWriter()
{
    delete &par;
    delete conn;
    delete writerowrange;
    delete writecolrange;
    delete ioobj;
}


int EM::dgbSurfaceWriter::nrPatches() const
{
    return surface.nrPatches();
}


EM::PatchID EM::dgbSurfaceWriter::patchID( int idx ) const
{
    return surface.patchID(idx);
}


const char* EM::dgbSurfaceWriter::patchName( int idx ) const
{
    return surface.patchName(patchID(idx));
}


void EM::dgbSurfaceWriter::selPatches(const TypeSet<EM::PatchID>& sel)
{
    patchsel = sel;
}


int EM::dgbSurfaceWriter::nrAuxVals() const
{
    return surface.nrAuxData();
}


const char* EM::dgbSurfaceWriter::auxDataName( int idx ) const
{
    return surface.auxDataName(idx);
}


void EM::dgbSurfaceWriter::selAuxData(const TypeSet<int>& sel )
{
    auxdatasel = sel;
}


const StepInterval<int>& EM::dgbSurfaceWriter::rowInterval() const
{
    return rowrange;
}


const StepInterval<int>& EM::dgbSurfaceWriter::colInterval() const
{
    return colrange;
}


void EM::dgbSurfaceWriter::setRowInterval( const StepInterval<int>& rg )
{
    if ( writerowrange ) delete writerowrange;
    writerowrange = new StepInterval<int>(rg);
}


void EM::dgbSurfaceWriter::setColInterval( const StepInterval<int>& rg )
{
    if ( writecolrange ) delete writecolrange;
    writecolrange = new StepInterval<int>(rg);
}


bool EM::dgbSurfaceWriter::writeOnlyZ() const
{
    return writeonlyz;
}


void EM::dgbSurfaceWriter::setWriteOnlyZ(bool yn)
{
    writeonlyz = yn;
}


IOPar* EM::dgbSurfaceWriter::pars()
{
    return &par;
}


int EM::dgbSurfaceWriter::nrDone() const
{
    return (executors.size() ? ExecutorGroup::nrDone() : 0) + nrdone;
}


const char* EM::dgbSurfaceWriter::nrDoneText() const
{
    return "Gridlines written";
}


int EM::dgbSurfaceWriter::totalNr() const
{
    return (executors.size() ? ExecutorGroup::totalNr() : 0) + 
	   (writerowrange?writerowrange->nrSteps():rowrange.nrSteps()) *
	   patchsel.size();
}


int EM::dgbSurfaceWriter::nextStep()
{
    if ( !ioobj ) { msg = "No object info"; return -1; }
    static const char* writeerror = "Cannot write surface";

    if ( !nrdone )
    {
	conn = dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Write));
	if ( !conn )
	{
	    msg = "Cannot open output surface file";
	    return ErrorOccurred;
	}

	for ( int idx=0; idx<auxdatasel.size(); idx++ )
	{
	    if ( auxdatasel[idx]>=surface.nrAuxData() )
		continue;

	    BufferString fnm( dgbSurfDataWriter::createHovName( 
		    			conn->fileName(),auxdatasel[idx]) );
	    add( new dgbSurfDataWriter(surface,auxdatasel[idx],0,binary,fnm) ); 
	    // TODO:: Change binid sampler so not all values are written when
	    // there is a subselection
	}

	par.set( EM::dgbSurfaceReader::rowrangestr,
		 writerowrange->start,writerowrange->stop,writerowrange->step);
	par.set( EM::dgbSurfaceReader::colrangestr,
		 writecolrange->start,writecolrange->stop,writecolrange->step);

	par.set( dgbSurfaceReader::nrpatchstr, patchsel.size() );
	for ( int idx=0; idx<patchsel.size(); idx++ )
	{
	    BufferString key( dgbSurfaceReader::patchidstr );
	    key += idx;
	    par.set( key, patchsel[idx] );

	    key = dgbSurfaceReader::patchnamestr;
	    key += idx;
	    par.set( key, surface.patchName(patchsel[idx]));
	}

	std::ostream& stream = conn->oStream();
	ascostream astream( stream );
	astream.putHeader( filetype );
	par.putTo( astream );
    }

    if ( patchindex>=patchsel.size() )
    {
	const int res = ExecutorGroup::nextStep();
	if ( !res ) const_cast<EM::Surface*>(&surface)->resetChangedFlag();
	return res;
    }

    std::ostream& stream = conn->oStream();

    static const char* tab = "\t";
    static const char* eol = "\n";
    static const char* eoltab = "\n\t\t";

    if ( patchindex!=oldpatchindex )
    {
	const Geometry::MeshSurface* gsurf =
	    			surface.getSurface(patchsel[patchindex]);

	StepInterval<int> patchrange;
       	surface.getRange( patchsel[patchindex], patchrange, true );
	firstrow = patchrange.start;
	int lastrow = patchrange.stop;

	if ( writerowrange )
	{
	    if ( firstrow>writerowrange->stop || lastrow<writerowrange->start)
		nrrows = 0;
	    else
	    {
		if ( firstrow<writerowrange->start )
		    firstrow = writerowrange->start;

		if ( lastrow>writerowrange->stop )
		    lastrow = writerowrange->stop;

		firstrow = writerowrange->snap( firstrow );
		lastrow = writerowrange->snap( lastrow );

		nrrows = (lastrow-firstrow)/writerowrange->step+1;
	    }

	}
	else
	{
	    nrrows = (lastrow-firstrow)/rowrange.step+1;
	}

	if ( !writeInt(stream,nrrows,nrrows ? tab : eol ) )
	{
	    msg = writeerror;
	    return ErrorOccurred;
	}

	if ( !nrrows )
	{
	    patchindex++;
	    nrdone++;
	    return MoreToDo;
	}

	if ( !writeInt(stream,firstrow,eol) )
	{
	    msg = writeerror;
	    return ErrorOccurred;
	}

	oldpatchindex = patchindex;
	rowindex = 0;
    }

    const int row = firstrow+rowindex *
		    (writerowrange?writerowrange->step:rowrange.step);

    const EM::PatchID patchid = surface.patchID(patchindex);
    TypeSet<Coord3> colcoords;

    int firstcol = -1;
    const int nrcols =
	(writecolrange?writecolrange->nrSteps():colrange.nrSteps())+1;
    for ( int colindex=0; colindex<nrcols; colindex++ )
    {
	const int col = writecolrange ? writecolrange->atIndex(colindex) :
	    				colrange.atIndex(colindex);

	const EM::PosID posid(  surface.id(), patchid,
				surface.rowCol2SubID(RowCol(row,col)));
	const Coord3 pos = surface.getPos(posid);

	if ( !colcoords.size() && !pos.isDefined() )
	    continue;

	if ( !colcoords.size() )
	    firstcol = col;

	colcoords += pos;
    }

    for ( int idx=colcoords.size()-1; idx>=0; idx-- )
    {
	if ( colcoords[idx].isDefined() )
	    break;

	colcoords.remove(idx);
    }

    if ( !writeInt(stream,colcoords.size(),colcoords.size()?tab:eol) )
    {
	msg = writeerror;
	return ErrorOccurred;
    }


    if ( colcoords.size() )
    {
	if ( !writeInt(stream,firstcol,tab) )
	{
	    msg = writeerror;
	    return ErrorOccurred;
	}

	for ( int idx=0; idx<colcoords.size(); idx++ )
	{
	    const Coord3 pos = colcoords[idx];
	    if ( !writeonlyz )
	    {
		if ( !writeFloat(stream,pos.x,tab) ||
			!writeFloat(stream,pos.y,tab))
		{
		    msg = writeerror;
		    return ErrorOccurred;
		}
	    }

	    if ( !writeFloat(stream,pos.z,idx!=colcoords.size()-1?eoltab:eol) )
	    {
		msg = writeerror;
		return ErrorOccurred;
	    }
	}
    }
	    
    rowindex++;
    if ( rowindex>=nrrows )
    {
	patchindex++;
	stream.flush();
    }

    nrdone++;
    return MoreToDo;
}


const char* EM::dgbSurfaceWriter::message() const
{
    return msg;
}


bool EM::dgbSurfaceWriter::writeInt(std::ostream& strm, int val,
				    const char* post) const
{
    if ( binary )
	strm.write((const char*)&val,sizeof(val));
    else
	strm << val << post;;

    return strm;
}


bool EM::dgbSurfaceWriter::writeFloat( std::ostream& strm, float val,
				       const char* post) const
{
    if ( binary )
	strm.write((const char*) &val,sizeof(val));
    else
	strm << val << post;;

    return strm;
}

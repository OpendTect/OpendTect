/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emsurfaceauxdata.cc,v 1.3 2004-09-01 12:51:40 nanne Exp $";

#include "emsurfaceauxdata.h"

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "geommeshsurface.h"
#include "ioman.h"
#include "iopar.h"
#include "ioobj.h"
#include "ptrman.h"
#include "settings.h"
#include "executor.h"
#include "filegen.h"

namespace EM
{

SurfaceAuxData::SurfaceAuxData( Surface& surf )
    : surface( surf )
    , changed( 0 )
{
    auxdatanames.allowNull(true);
    auxdatainfo.allowNull(true);
    auxdata.allowNull(true);
}


SurfaceAuxData::~SurfaceAuxData()
{
    removeAll();
}


void SurfaceAuxData::removeAll()
{
    deepErase( auxdatanames );
    deepErase( auxdatainfo );
    for ( int idx=0; idx<auxdata.size(); idx++ )
    {
	if ( !auxdata[idx] ) continue;
	deepErase( *auxdata[idx] );
    }

    deepErase( auxdata );
    changed = true;
}


int SurfaceAuxData::nrAuxData() const
{ return auxdatanames.size(); }


const char* SurfaceAuxData::auxDataName( int dataidx ) const
{
    if ( nrAuxData() && auxdatanames[dataidx] )
	return *auxdatanames[dataidx];

    return 0;
}


void SurfaceAuxData::setAuxDataName( int dataidx, const char* name )
{
    if ( auxdatanames[dataidx] )
	auxdatanames.replace( new BufferString(name), dataidx );
}


int SurfaceAuxData::auxDataIndex( const char* nm ) const
{
    for ( int idx=0; idx<auxdatanames.size(); idx++ )
	if ( *auxdatanames[idx] == nm ) return idx;
    return -1;
}


int SurfaceAuxData::addAuxData( const char* name )
{
    auxdatanames += new BufferString( name );
    ObjectSet<TypeSet<float> >* newauxdata = new ObjectSet<TypeSet<float> >;
    auxdata += newauxdata;
    newauxdata->allowNull(true);

    for ( int idx=0; idx<surface.geometry.nrSections(); idx++ )
	(*newauxdata) += 0;

    changed = true;
    return auxdatanames.size()-1;
}


void SurfaceAuxData::removeAuxData( int dataidx )
{
    delete auxdatanames[dataidx];
    auxdatanames.replace( 0, dataidx );

    deepEraseArr( *auxdata[dataidx] );
    delete auxdata[dataidx];
    auxdata.replace( 0, dataidx );
    changed = true;
}


float SurfaceAuxData::getAuxDataVal( int dataidx, const PosID& posid ) const
{
    if ( !auxdata[dataidx] ) return mUndefValue;
    const int sectionidx = surface.geometry.sectionNr( posid.sectionID() );
    if ( sectionidx==-1 ) return mUndefValue;

    const TypeSet<float>* sectionauxdata = sectionidx<auxdata[dataidx]->size()
	? (*auxdata[dataidx])[sectionidx] : 0;

    if ( !sectionauxdata ) return mUndefValue;

    RowCol geomrc;
    surface.geometry.getMeshRowCol( posid.subID(), geomrc, posid.sectionID() );
    const int subidx =
	surface.geometry.getSurface(posid.sectionID())->indexOf(geomrc);
    if ( subidx==-1 ) return mUndefValue;
    return (*sectionauxdata)[subidx];
}


void SurfaceAuxData::setAuxDataVal( int dataidx, const PosID& posid, float val)
{
    if ( !auxdata[dataidx] ) return;

    const int sectionidx = surface.geometry.sectionNr( posid.sectionID() );
    if ( sectionidx==-1 ) return;

    RowCol geomrc; 
    surface.geometry.getMeshRowCol( posid.subID(), geomrc, posid.sectionID() );
    const int subidx =
	surface.geometry.getSurface(posid.sectionID())->indexOf(geomrc);
    if ( subidx==-1 ) return;

    TypeSet<float>* sectionauxdata = sectionidx<auxdata[dataidx]->size()
	? (*auxdata[dataidx])[sectionidx] : 0;
    if ( !sectionauxdata )
    {
	for ( int idx=auxdata[dataidx]->size(); idx<=sectionidx; idx++ )
	    (*auxdata[dataidx]) += 0;

	const int sz = surface.geometry.getSurface(posid.sectionID())->size();
	auxdata[dataidx]->replace( new TypeSet<float>(sz,mUndefValue), 
				   sectionidx );
	sectionauxdata = (*auxdata[dataidx])[sectionidx];
    }

    (*sectionauxdata)[subidx] = val;
    changed = true;
}


bool SurfaceAuxData::isChanged(int idx) const
{ return changed; }


void SurfaceAuxData::resetChangedFlag()
{
    changed = false;
}


Executor* SurfaceAuxData::auxDataLoader( int selidx )
{
    PtrMan<IOObj> ioobj = IOM().get( surface.multiID() );
    if ( !ioobj )
	{ surface.errmsg = "Cannot find surface"; return 0; }
    StreamConn* conn = dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
    if ( !conn ) return 0;

    PtrMan<EMSurfaceTranslator> tr = 
			(EMSurfaceTranslator*)ioobj->getTranslator();
    if ( !tr || !tr->startRead(*ioobj) )
    { surface.errmsg = tr ? tr->errMsg() : "Cannot find Translator"; return 0; }

    SurfaceIODataSelection& sel = tr->selections();
    int nrauxdata = sel.sd.valnames.size();
    if ( !nrauxdata || selidx >= nrauxdata ) return 0;

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes reader" );
    for ( int validx=0; validx<sel.sd.valnames.size(); validx++ )
    {
	if ( selidx>=0 && selidx != validx ) continue;

	const char* attrnm = sel.sd.valnames[validx]->buf();
	int gap = 0;
	for ( int idx=0; ; idx++ )
	{
	    if ( gap > 50 ) return 0;
	    BufferString fnm = 
		dgbSurfDataWriter::createHovName(conn->fileName(),idx);
	    if ( File_isEmpty(fnm) ) { gap++; continue; }
	    else gap = 0;

	    dgbSurfDataReader* rdr = new dgbSurfDataReader(fnm);
	    if ( strcmp(attrnm,rdr->dataName()) )
	    { delete rdr; continue; }

	    rdr->setSurface( surface );
	    grp->add( rdr );
	    break;
	}
    }

    return grp;
}


Executor* SurfaceAuxData::auxDataSaver( int dataidx, int fileidx )
{
    PtrMan<IOObj> ioobj = IOM().get( surface.multiID() );
    if ( !ioobj )
	{ surface.errmsg = "Cannot find surface"; return 0; }
    StreamConn* conn = dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
    if ( !conn ) return 0;

    bool binary = true;
    mSettUse(getYN,"dTect.Surface","Binary format",binary);

    BufferString fnm;
    if ( fileidx >= 0 )
    {
	if ( dataidx<0 ) dataidx = 0;
	fnm = dgbSurfDataWriter::createHovName( conn->fileName(), fileidx );
	return new dgbSurfDataWriter(surface,dataidx,0,binary,fnm);
    }

    ExecutorGroup* grp = new ExecutorGroup( "Surface attributes saver" );
    for ( int selidx=0; selidx<nrAuxData(); selidx++ )
    {
	if ( dataidx >= 0 && dataidx != selidx ) continue;
	for ( int idx=0; ; idx++ )
	{
	    fnm = dgbSurfDataWriter::createHovName( conn->fileName(), idx );
	    if ( !File_exists(fnm) )
		break;
	}

	Executor* exec = new dgbSurfDataWriter(surface,selidx,0,binary,fnm);
	grp->add( exec );
    }

    return grp;
}


void SurfaceAuxData::removeSection(const SectionID& sectionid)
{
    const int sectionidx = surface.geometry.sectionNr(sectionid);
    if ( sectionidx==-1 ) return;

    for ( int idy=0; idy<nrAuxData(); idy++ )
    {
	if ( !auxdata[idy] )
	    continue;

	delete (*auxdata[idy])[sectionidx];
	auxdata[idy]->replace( 0, sectionidx );
    }
}


bool SurfaceAuxData::usePar( const IOPar& par )
{
    return true;
}


void SurfaceAuxData::fillPar( IOPar& par ) const
{
}

}; //namespace

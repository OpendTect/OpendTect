/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	nageswara
 Date:		March 2018
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "segyvintageimporter.h"

#include "dbman.h"
#include "keystrs.h"
#include "repos.h"
#include "seisimporter.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survgeom.h"

SEGY::Vintage::Importer::Importer( const SEGY::Vintage::Info& vntinfo,
				   const OD::String& trnalnm,
				   const Seis::GeomType gt, Seis::SelData* sd )
	: ExecutorGroup( vntinfo.vintagenm_ )
{
    Repos::IOParSet parset = Repos::IOParSet( "SEGYSetups" );
    const int selidx = parset.find( vntinfo.vintagenm_ );
    Repos::IOPar* iop = parset[selidx];
    FullSpec fullspec( gt, false);
    IOObjContext ctxt = mIOObjContext(SeisTrc);
    ctxt.fixTranslator( trnalnm );
    const int size = vntinfo.filenms_.size();
    for ( int nmidx=0; nmidx<size; nmidx++ )
    {
	CtxtIOObj ctio( ctxt );
	const BufferString fnm( vntinfo.filenms_.get( nmidx ) );
	const File::Path fp( vntinfo.fp_.pathOnly(), fnm );
	ctio.setName( fp.baseName() );
	DBM().getEntry( ctio );
	if ( !ctio.ioobj_ )
	    continue;

	iop->set( sKey::FileName(), fp.fullPath() );
	fullspec.usePar( *iop );
	IOObj* inioobj = fullspec.spec_.getIOObj( true );
	if ( !inioobj )
	    continue;

	DBM().setEntry(*inioobj);

	SeisTrcWriter* writer = new SeisTrcWriter( ctio.ioobj_ );
	SeisStdImporterReader* rdr =
				new SeisStdImporterReader( *inioobj, "SEG-Y" );
	if ( sd )
	{
	    sd->setGeomID( mUdfGeomID );
	    rdr->setSelData( sd->clone() );
	    writer->setSelData( sd->clone() );
	}

	SeisImporter* imp = new SeisImporter( rdr, *writer, gt );

	seistrcwriters_.add( writer );
	add( imp );
    }
}


SEGY::Vintage::Importer::~Importer()
{
    deepErase( seistrcwriters_ );
}

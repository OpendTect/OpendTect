/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seis2dline.cc,v 1.2 2004-06-18 13:58:07 bert Exp $";

#include "seis2dline.h"
#include "seisbuf.h"
#include "survinfo.h"
#include "strmprov.h"
#include "ascstream.h"
#include "keystrs.h"
#include "iopar.h"
#include "errh.h"

const char* Seis2DLineGroup::sKeyZRange = SurveyInfo::sKeyZRange;
const char* Seis2DLineIOProvider::sKeyType = sKey::Type;
const char* Seis2DLineIOProvider::sKeyLineNr = "Line number";


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs()
{
    static ObjectSet<Seis2DLineIOProvider>* theinst = 0;
    if ( !theinst ) theinst = new ObjectSet<Seis2DLineIOProvider>;
    return *theinst;
}


Seis2DLineGroup::~Seis2DLineGroup()
{
    deepErase( pars_ );
}


Seis2DLineGroup& Seis2DLineGroup::operator =( const Seis2DLineGroup& lg )
{
    if ( &lg == this ) return *this;
    fname_ = lg.fname_;
    readFile();
    return *this;
}


void Seis2DLineGroup::init( const char* fnm )
{
    fname_ = fnm;
    readFile();
}


static const char* sKeyFileType = "2D Line Group Data";

void Seis2DLineGroup::readFile()
{
    deepErase( pars_ );

    StreamData sd = StreamProvider( fname_ ).makeIStream();
    if ( !sd.usable() ) return;

    ascistream astrm( *sd.istrm, true );
    if ( !astrm.isOfFileType( sKeyFileType ) )
	{ sd.close(); return; }

    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    setName( astrm.value() );
    }

    while ( !astrm.type() == ascistream::EndOfFile )
    {
	IOPar* newpar = new IOPar;
	while ( !atEndOfSection(astrm.next()) )
	    newpar->set( astrm.keyWord(), astrm.value() );
	if ( newpar->size() < 1 )
	    delete newpar;
	else
	    pars_ += newpar;
    }

    sd.close();
}


void Seis2DLineGroup::writeFile() const
{
    StreamData sd = StreamProvider( fname_ ).makeOStream();
    if ( !sd.usable() ) return;

    ascostream astrm( *sd.ostrm, true );
    if ( !astrm.putHeader( sKeyFileType ) )
	{ sd.close(); return; }

    astrm.put( sKey::Name, name() );
    astrm.put( "Number of lines", pars_.size() );
    astrm.newParagraph();

    for ( int ipar=0; ipar<pars_.size(); ipar++ )
    {
	const IOPar& iopar = *pars_[ipar];
	for ( int idx=0; idx<iopar.size(); idx++ )
	    astrm.put( iopar.getKey(idx), iopar.getValue(idx) );
	astrm.newParagraph();
    }

    sd.close();
}


Seis2DLineIOProvider* Seis2DLineGroup::getLiop( int ipar ) const
{
    return ipar >= pars_.size() ? 0 : getLiop( *pars_[ipar] );
}


Seis2DLineIOProvider* Seis2DLineGroup::getLiop( const IOPar& iop ) const
{
    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOPs();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	Seis2DLineIOProvider* liop = liops[idx];
	if ( liop->isUsable(iop) )
	    return liop;
    }
    return 0;
}


Executor* Seis2DLineGroup::lineFetcher( int ipar, SeisTrcBuf& tbuf ) const
{
    Seis2DLineIOProvider* liop = getLiop( ipar );
    if ( !liop )
    {
	ErrMsg("No suitable 2D line extraction object found");
	return 0;
    }

    return liop->getFetcher( *pars_[ipar], tbuf );
}


Executor* Seis2DLineGroup::lineAdder( IOPar* newiop,
				      const SeisTrcBuf& tbuf ) const
{
    if ( !newiop || !newiop->size() || !tbuf.size() )
    {
	ErrMsg("No data for line add provided");
	return 0;
    }
    Seis2DLineIOProvider* liop = getLiop( *newiop );
    if ( !liop )
    {
	ErrMsg("No suitable 2D line creation object found");
	return 0;
    }

    const IOPar* previop = pars_.size() ? pars_[pars_.size()-1] : 0;
    return liop->getPutter( *newiop, tbuf, previop );
}


void Seis2DLineGroup::commitAdd( IOPar* newiop )
{
    pars_ += newiop;
    writeFile();
}


bool Seis2DLineGroup::isEmpty( int ipar ) const
{
    Seis2DLineIOProvider* liop = getLiop( ipar );
    return liop ? liop->isEmpty( *pars_[ipar] ) : true;
}


void Seis2DLineGroup::remove( int idx )
{
    if ( idx > pars_.size() ) return;

    IOPar* iop = pars_[idx];
    pars_ -= iop;
    delete iop;
    writeFile();
}


bool Seis2DLineIOProvider::isUsable( const IOPar& iop ) const
{
    const char* res = iop.find( sKeyType );
    return res && type == res;
}

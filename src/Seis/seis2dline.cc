/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seis2dline.cc,v 1.1 2004-06-17 14:56:51 bert Exp $";

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


ObjectSet<Seis2DLineIOProvider>& S2DLIOP()
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

    for ( int ipar=0; ipar<pars_.size(); ipar++ )
    {
	const IOPar& iopar = *pars_[ipar];
	for ( int idx=0; idx<iopar.size(); idx++ )
	    astrm.put( iopar.getKey(idx), iopar.getValue(idx) );
	astrm.newParagraph();
    }

    sd.close();
}


SeisTrcBuf* Seis2DLineGroup::getData( int paridx ) const
{
    if ( paridx > pars_.size() ) return 0;

    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOP();
    const IOPar& iopar = *pars_[paridx];

    for ( int idx=0; idx<liops.size(); idx++ )
    {
	Seis2DLineIOProvider& liop = *liops[idx];
	if ( liop.isUsable(iopar) )
	{
	    SeisTrcBuf* ret = liop.getData( iopar );
	    if ( liop.errmsg != "" )
		ErrMsg( liop.errmsg );
	    return ret;
	}
    }

    return 0;
}


void Seis2DLineGroup::add( IOPar* newiop, const SeisTrcBuf& tbuf )
{
    if ( !newiop || !newiop->size() || !tbuf.size() ) return;

    const IOPar* previop = pars_.size() ? pars_[pars_.size()-1] : 0;

    const ObjectSet<Seis2DLineIOProvider>& liops = S2DLIOP();
    for ( int idx=0; idx<liops.size(); idx++ )
    {
	Seis2DLineIOProvider& liop = *liops[idx];
	if ( liop.isUsable(*newiop) )
	{
	    if ( !liop.putData(*newiop,tbuf,previop) )
		ErrMsg( liop.errmsg );
	    else
		break;
	}
	if ( idx == liops.size()-1 )
	    return;
    }

    pars_ += newiop;
    writeFile();
}


bool Seis2DLineIOProvider::isUsable( const IOPar& iop ) const
{
    const char* res = iop.find( sKeyType );
    return res && type == res;
}

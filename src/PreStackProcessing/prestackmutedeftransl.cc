/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackmutedeftransl.h"

#include "ascstream.h"
#include "binid.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "keystrs.h"
#include "mathfunc.h"
#include "prestackmutedef.h"
#include "ptrman.h"
#include "ioman.h"
#include "streamconn.h"
#include "uistrings.h"

static const char* sKeyPBMFSetup = "PBMF setup";

// MuteDefTranslatorGroup

defineTranslatorGroup(MuteDef,"Mute Definition");
uiString MuteDefTranslatorGroup::sTypeName(int num)
{ return uiStrings::sMute(num); }

defineTranslator(dgb,MuteDef,mDGBKey);

mDefSimpleTranslatorioContext(MuteDef,Misc)
mDefSimpleTranslatorSelector(MuteDef);

MuteDefTranslatorGroup::MuteDefTranslatorGroup()
    : TranslatorGroup("MuteDef")
{
}


// MuteDefTranslator

MuteDefTranslator::MuteDefTranslator( const char* nm, const char* unm )
    : Translator(nm,unm)
{
}


MuteDefTranslator::~MuteDefTranslator()
{
}


uiString MuteDefTranslator::sSelObjNotMuteDef()
{
    return tr("Selected object is not a Mute Definition");
}


bool MuteDefTranslator::retrieve( PreStack::MuteDef& md, const IOObj* ioobj,
				  uiString& msg )
{
    if ( !ioobj ) { msg = uiStrings::sCantFindODB(); return false; }
    mDynamicCast(MuteDefTranslator*,PtrMan<MuteDefTranslator> mdtrl,
		 ioobj->createTranslator());
    if ( !mdtrl )
	{ msg = sSelObjNotMuteDef(); return false; }

    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
    {
	msg = uiStrings::phrCannotOpen(toUiString(ioobj->fullUserExpr(true)));
	return false;
    }

    msg = mToUiStringTodo( mdtrl->read( md, *conn ) );
    return msg.isEmpty();
}


bool MuteDefTranslator::store( const PreStack::MuteDef& md, const IOObj* ioobj,
				uiString& msg )
{
    if ( !ioobj )
    {
	msg = sNoIoobjMsg();
	return false;
    }
    mDynamicCast(MuteDefTranslator*,PtrMan<MuteDefTranslator> mdtrl,
		 ioobj->createTranslator());
    if ( !mdtrl )
    {
	msg = sSelObjNotMuteDef();
	return false;
    }

    msg.setEmpty();
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
    {
	msg = uiStrings::phrCannotOpen(toUiString(ioobj->fullUserExpr(false)));
    }
    else
    {
	msg = mToUiStringTodo( mdtrl->write( md, *conn ) );
    }

    return msg.isEmpty();
}


// dgbMuteDefTranslator

dgbMuteDefTranslator::dgbMuteDefTranslator( const char* nm, const char* unm )
    : MuteDefTranslator(nm,unm)
{
}


const char* dgbMuteDefTranslator::read( PreStack::MuteDef& md, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    if ( !astrm.isOK() )
	return "Cannot read from input file";
    else if ( !astrm.isOfFileType(mTranslGroupName(MuteDef)) )
	return "Input file is not a Mute Definition file";

    const bool hasiopar = hasIOPar( astrm.majorVersion(),
				    astrm.minorVersion() );

    if ( hasiopar )
    {
	IOPar pars( astrm );
	MultiID hormid;
	pars.get( sKeyRefHor(), hormid );
	md.setReferenceHorizon( hormid );
    }

    if ( atEndOfSection(astrm) ) astrm.next();
    if ( atEndOfSection(astrm) ) astrm.next(); //skip for anextraparagraph that
					//was inserted between version 4.4 & 4.6
    if ( atEndOfSection(astrm) )
	return "Input file contains no Mute Definition locations";

    while ( md.size() ) md.remove( 0 );
    md.setName( IOM().nameOf(conn.linkedTo()) );

    for ( int ifn=0; !atEndOfSection(astrm); ifn++ )
    {
	if ( astrm.hasKeyword(sKeyRefHor()) )
	{
	    MultiID hormid = astrm.value();
	    md.setReferenceHorizon( hormid );
	    astrm.next();
	}

	BinID bid;
	bool extrapol = true;
	bool rejectpt = false;
	PointBasedMathFunction::InterpolType it =PointBasedMathFunction::Linear;

	if ( astrm.hasKeyword(sKey::Position()) )
	{
	    bid.fromString( astrm.value() );
	    if ( !bid.inl() || !bid.crl() )
		rejectpt = true;

	    astrm.next();
	}
	if ( astrm.hasKeyword(sKeyPBMFSetup) )
	{
	    const char* val = astrm.value();
	    if ( *val )
	    {
		it = (*val == 'S' ? PointBasedMathFunction::Snap
		   : (*val == 'P' ? PointBasedMathFunction::Poly
				  : PointBasedMathFunction::Linear));
		extrapol = *(val+1) != 'N';
	    }
	    astrm.next();
	}

	const PointBasedMathFunction::ExtrapolType et = extrapol
	    ? PointBasedMathFunction::EndVal
	    : PointBasedMathFunction::None;
	PointBasedMathFunction* fn = new PointBasedMathFunction( it, et );
	while ( !atEndOfSection(astrm) )
	{
	    BufferString val( astrm.keyWord() );
	    char* ptr1stval = val.getCStr();
	    mSkipBlanks(ptr1stval);
	    char* ptr2ndval = ptr1stval;
	    mSkipNonBlanks( ptr2ndval );
	    if ( !*ptr2ndval )
		{ astrm.next(); continue; }
	    *ptr2ndval = '\0'; ptr2ndval++;
	    mSkipBlanks(ptr2ndval);
	    if ( !*ptr2ndval )
		{ astrm.next(); continue; }

	    float firstval = toFloat( ptr1stval );
	    float secondval = toFloat( ptr2ndval );
	    if ( !mIsUdf(firstval) && !mIsUdf(secondval) )
	    {
		/*if ( version < 440 )
		    fn->add( secondval, firstval );
		else TODO have to recheck later on*/
		fn->add( firstval, secondval );
	    }

	    astrm.next();
	}

	if ( rejectpt || fn->size()<1 )
	    delete fn;
	else
	    md.add( fn, bid );

	astrm.next();
    }

    if ( md.size() )
    {
	md.setChanged( false );
	return 0;
    }

    return "No valid mute points found";
}


bool dgbMuteDefTranslator::hasIOPar(int majorversion, int minorversion )
{
    if ( majorversion<3 )
	return false;
    if ( majorversion>4 )
	return true;

    return minorversion>3;
}


const char* dgbMuteDefTranslator::write( const PreStack::MuteDef& md,Conn& conn)
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(MuteDef) );

    const bool hasiopar = hasIOPar( mODMajorVersion, mODMinorVersion );

    if ( hasiopar )
    {
	IOPar pars;
	pars.set( sKeyRefHor(), md.getReferenceHorizon() );
	pars.putTo( astrm );
    }

    od_ostream& strm = astrm.stream();
    if ( !strm.isOK() )
	return "Cannot write to output Mute Definition file";

    for ( int imd=0; imd<md.size(); imd++ )
    {
	if ( !imd && !hasiopar )
	    astrm.put( sKeyRefHor(), md.getReferenceHorizon() );

	astrm.put( sKey::Position(), md.getPos(imd).toString() );
	const PointBasedMathFunction& pbmf = md.getFn( imd );
	char buf[3];
	buf[0] =  pbmf.interpolType() == PointBasedMathFunction::Snap
	 ? 'S' : (pbmf.interpolType() == PointBasedMathFunction::Poly
	 ? 'P' : 'L');
	buf[1] = pbmf.extrapolate() ? 'Y' : 'N';
	buf[2] = '\0';
	astrm.put( sKeyPBMFSetup, buf );

	for ( int idx=0; idx<pbmf.size(); idx++ )
	    strm << pbmf.xVals()[idx] << '\t' << pbmf.yVals()[idx] << '\n';

	astrm.newParagraph();
    }

    if ( strm.isOK() )
    {
	const_cast<PreStack::MuteDef&>(md).setChanged( false );
	return 0;
    }

    return "Error during write to output Mute Definition file";
}

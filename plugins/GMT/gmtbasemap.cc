/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "color.h"
#include "draw.h"
#include "file.h"
#include "filepath.h"
#include "gmtbasemap.h"
#include "initgmtplugin.h"
#include "keystrs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "od_iostream.h"


static const int cTitleBoxHeight = 4;
static const int cTitleBoxWidth = 8;

namespace GMT {

static void setDefaults( OS::MachineCommand& mc )
{
    const bool moderngmt = GMT::hasModernGMT();

    const BufferString fontstr( moderngmt ? "FONT_TITLE" : "HEADER_FONT_SIZE",
				"=24" );
    const BufferString annotstr( moderngmt ? "FONT_ANNOT" : "ANNOT_FONT_SIZE",
				 "_PRIMARY=12" );
    const BufferString lblstr( moderngmt ? "FONT_LABEL" : "LABEL_FONT_SIZE",
			       "=12" );
    const BufferString ftannstr(
		moderngmt ? "FONT_ANNOT_PRIMARY" : "ANNOT_FONT_SIZE_PRIMARY",
		"=10" );

    mc.addFlag( fontstr ).addFlag( annotstr );
    if ( !moderngmt )
	mc.addFlag( "FRAME_PEN=2p" );

    mc.addFlag( lblstr ).addFlag( ftannstr );
}

};

int GMTBaseMap::factoryid_ = -1;

void GMTBaseMap::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Basemap", GMTBaseMap::createInstance );
}

GMTPar* GMTBaseMap::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTBaseMap( iop, workdir );
}


bool GMTBaseMap::doExecute( od_ostream& strm, const char* fnm )
{
    strm << "Creating the Basemap ...  ";
    StringView maptitle = find( ODGMT::sKeyMapTitle() );
    Interval<float> lblintv;
    if ( !get(ODGMT::sKeyLabelIntv(),lblintv) )
	mErrStrmRet("Incomplete data for basemap creation")

    bool closeps = false, dogrid = false;
    getYN( ODGMT::sKeyClosePS(), closeps );
    getYN( ODGMT::sKeyDrawGridLines(), dogrid );
    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")
    get( ODGMT::sKeyMapDim(), mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    const float pagewidth = mapdim.start + 5 * xmargin;
    const float pageheight = mapdim.stop + 3 * ymargin;
    int scaleval = 1;
    get( ODGMT::sKeyMapScale(), scaleval );
    BufferStringSet remset;
    get( ODGMT::sKeyRemarks(), remset );

    const bool moderngmt = GMT::hasModernGMT();

    BufferString lblrgstr( "-Ba", lblintv.start );
    if ( dogrid ) lblrgstr.add( "g" ).add( lblintv.start );
    lblrgstr.add( "/a" ).add( lblintv.stop );
    if ( dogrid ) lblrgstr.add( "g" ).add( lblintv.stop );
    lblrgstr.add( ":\'." ).add( maptitle ).add( "\':" );
    BufferString mapxorigstr( moderngmt ? "MAP_ORIGIN_X=" : "X_ORIGIN=" );
    mapxorigstr.add( xmargin ).add( "c" );
    BufferString mapyorigstr( moderngmt ? "MAP_ORIGIN_Y=" : "Y_ORIGIN=" );
    mapyorigstr.add( ymargin ).add( "c" );
    BufferString paperstr( moderngmt ? "PS" : "PAPER", "_MEDIA=Custom_" );
    paperstr.add( pageheight < 21 ? 21 : pageheight ).add( "cx" )
	    .add( pagewidth < 21 ? 21 : pagewidth ).add( "c" );

    OS::MachineCommand bsmpmc( "psbasemap" );
    bsmpmc.addArg( mapprojstr ).addArg( rgstr ).addArg( lblrgstr )
	  .addFlag( moderngmt ? "MAP_ANNOT_ORTHO=z" : "Y_AXIS_TYPE=ver_text" );
    GMT::setDefaults( bsmpmc );
    bsmpmc.addFlag( mapxorigstr ).addFlag( mapyorigstr )
	  .addFlag( paperstr ).addArg( "-K" );
    if ( !execCmd(bsmpmc,strm,fnm,false) )
	mErrStrmRet("Failed to create Basemap")

    strm << "Done" << od_endl;

    BufferString legendstr( "-Dx" );
    legendstr.add( mapdim.start + xmargin ).add( "c/" ).add( 0 ).add( "c" )
	     .add( moderngmt ? "+w" : "/" ).add( cTitleBoxWidth ).add( "c/" )
	     .add( cTitleBoxHeight ).add( "c" ).add( moderngmt ? "+j" : "/" )
	     .add( "BL" );
    BufferString cornerstr( "-UBL/" );
    cornerstr.add( moderngmt ? mapdim.start + xmargin : 0 ).add( "c/0c" );

    strm << "Posting title box ...  ";
    OS::MachineCommand lgdmc( "pslegend" );
    lgdmc.addArg( mapprojstr ).addArg( rgstr ).addArg( "-O" );
    if ( !closeps ) lgdmc.addArg( "-K" );
    lgdmc.addArg( "-F" ).addArg( legendstr ).addArg( cornerstr );
    od_ostream procstrm = makeOStream( lgdmc, strm, fnm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed to overlay title box")

    procstrm << "H 16 4 " << maptitle.buf() << "\n";
    procstrm << "G 0.5l" << "\n";
    procstrm << "L 10 4 C Scale  1:" << scaleval << "\n";
    procstrm << "D 0 1p" << "\n";
    for ( int idx=0; idx<remset.size(); idx++ )
	procstrm << "L 12 4 C " << remset.get(idx) << "\n";

    strm << "Done" << od_endl;
    return true;
}


int GMTLegend::factoryid_ = -1;

void GMTLegend::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Legend", GMTLegend::createInstance );
}

GMTPar* GMTLegend::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTLegend( iop, workdir );
}


bool GMTLegend::doExecute( od_ostream& strm, const char* fnm )
{
    strm << "Posting legends ...  ";
    bool hascolbar = false;
    ObjectSet<IOPar> parset;
    int parwithcolorbar = -1;
    for ( int idx=0; idx<100; idx++ )
    {
	IOPar* par = subselect( idx );
	if ( !par ) break;

	if ( par->find(ODGMT::sKeyPostColorBar()) )
	    parwithcolorbar = idx;

	parset += par;
    }

    const bool moderngmt = GMT::hasModernGMT();

    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")
    get( ODGMT::sKeyMapDim(), mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    if ( parwithcolorbar >= 0 )
    {
	hascolbar = true;
	const IOPar* par = parset[parwithcolorbar];
	StepInterval<float> rg;
	par->get( ODGMT::sKeyDataRange(), rg );
	BufferString ampstr( "-D" );
	ampstr.add( mapdim.start + xmargin )
	      .add( "c/" ).add( 1.2 * ymargin + cTitleBoxHeight )
	      .add( "c/" ).add( 2 * ymargin )
	      .add( "c/" ).add( xmargin / 2 ).add( "c" );
	BufferString bargstr( "-B" );
	bargstr.add( rg.step * 5 ).add( ":\'" ).add( par->find( sKey::Name() ) )
	       .add( "\':/:" ).add( par->find( ODGMT::sKeyAttribName() ) )
	       .add( ":" );

	FilePath cptfp( fnm );
	cptfp.setPath( getWorkDir() );
	cptfp.setExtension( "cpt" );
	BufferString cptfnm;
	cptfnm.set( cptfp.fileName() );
	if ( !cptfp.exists() )
	{
	    cptfnm.clean( BufferString::AllowDots );
	    cptfp.setFileName( cptfnm );
	}

	OS::MachineCommand colorbarmc( "psscale" );
	GMT::setDefaults( colorbarmc );
	colorbarmc.addArg( ampstr ).addArg( "-O" ).addArg( "-K" )
		  .addArg( BufferString("-C",cptfnm) )
		  .addArg( bargstr );
	if ( !execCmd(colorbarmc,strm,fnm) )
	    mErrStrmRet("Failed to post color bar");

	File::remove( cptfp.fullPath() );
    }

    const int nritems = parset.size();
    BufferString dargstr( "-Dx" );
    dargstr.add( mapdim.start + xmargin ).add( "c/" )
	   .add( ymargin / 2 + cTitleBoxHeight + (hascolbar ? 2 * ymargin : 0) )
	   .add( "c" ).add( moderngmt ? "+w" : "/" ).add( 10 ).add( "c/" )
	   .add( nritems ? nritems : 1 ).add( "c" )
	   .add( moderngmt ? "+j" : "/" ).add( "BL" );

    OS::MachineCommand lgdmc( "pslegend" );
    lgdmc.addArg( mapprojstr ).addArg( rgstr ).addArg( "-O" ).addArg( dargstr );
    od_ostream procstrm = makeOStream( lgdmc, strm, fnm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed to overlay legend")

    for ( int idx=0; idx<nritems; idx++ )
    {
	IOPar* par = parset[idx];
	StringView namestr = par->find( sKey::Name() );
	if ( namestr.isEmpty() )
	    continue;

	float sz = 1;
	BufferString symbstr, penstr;
	bool usewellsymbol = false;
	par->getYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbol );
	StringView shapestr = par->find( ODGMT::sKeyShape() );
	if ( !usewellsymbol && !shapestr ) continue;
	ODGMT::Shape shape = ODGMT::parseEnumShape( shapestr.str() );
	symbstr = ODGMT::sShapeKeys()[(int)shape];
	par->get( sKey::Size(), sz );
	if ( shape == ODGMT::Polygon || shape == ODGMT::Line )
	{
	    const char* lsstr = par->find( ODGMT::sKeyLineStyle() );
	    if ( !lsstr ) continue;

	    OD::LineStyle ls;
	    ls.fromString( lsstr );
	    if ( ls.type_ != OD::LineStyle::None )
	    {
		mGetLineStyleString( ls, penstr );
	    }
	    else if ( shape == ODGMT::Line )
		continue;
	}
	else
	{
	    OD::Color pencol;
	    par->get( sKey::Color(), pencol );
	    BufferString colstr;
	    mGetColorString( pencol, colstr );
	    penstr = "1p,"; penstr += colstr;
	}

	bool dofill;
	par->getYN( ODGMT::sKeyFill(), dofill );
	BufferString legendstring = "S 0.6c ";

	if ( !usewellsymbol )
	    legendstring += symbstr;
	else
	{
	    BufferString symbolname;
	    par->get( ODGMT::sKeyWellSymbolName(), symbolname );
	    BufferString deffilenm = GMTWSR().get( symbolname )->deffilenm_;
	    legendstring += "k"; legendstring += deffilenm;
	    par->get( sKey::Size(), sz );
	}

	legendstring += " ";
	legendstring += sz > 1 ? 1 : sz;
	legendstring += "c ";

	if ( !usewellsymbol && dofill )
	{
	    BufferString fillcolstr;
	    OD::Color fillcol;
	    par->get( ODGMT::sKeyFillColor(), fillcol );
	    mGetColorString( fillcol, fillcolstr );
	    legendstring += fillcolstr;
	}
	else
	    legendstring += "-";

	legendstring += " ";
	if ( penstr.isEmpty() )
	    legendstring += "-";
	else
	    legendstring += penstr;

	legendstring += " "; legendstring += 1.3;
	legendstring += " "; legendstring += namestr;
	DBG::message( DBG_PROGSTART, legendstring.str() );
	procstrm << legendstring << "\n";
	procstrm << "G0.2c" << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}



int GMTCommand::factoryid_ = -1;

void GMTCommand::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Advanced", GMTCommand::createInstance );
}

GMTPar* GMTCommand::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTCommand( iop, workdir );
}


const char* GMTCommand::userRef() const
{
    BufferString* str = new BufferString( "GMT Command: " );
    const char* res = find( ODGMT::sKeyCustomComm() );
    *str += res;
    *( str->getCStr() + 25 ) = '\0';
    return str->buf();
}


static char* parseNextWord( char* ptr, BufferString& arg )
{
    arg.setEmpty();
    if ( !ptr || !*ptr )
	return ptr;
    mSkipBlanks( ptr );
    if ( !*ptr )
	return ptr;

    bool inquotes = *ptr == '"';
    if ( inquotes )
	ptr++;

    for ( ; *ptr; ptr++ )
    {
	if ( *ptr == '"' )
	    { inquotes = !inquotes; continue; }

	const bool isescaped = *ptr == '\\';
	if ( isescaped )
	{
	    ptr++;
	    if ( !*ptr )
		break;
	    if ( *ptr == '\\' )
		arg.add( *ptr );
	}
	if ( *ptr == ' ' && !isescaped && !inquotes )
	    { ptr++; break; }

	arg.add( *ptr );
    }
    return ptr;
}


static bool setFromSingleStringRep( const char* inp, OS::MachineCommand& mc )
{
    BufferString pnm;
    BufferStringSet args;

    BufferString inpcomm( inp );
    inpcomm.trimBlanks();
    if ( inpcomm.isEmpty() )
	return false;

    char* ptr = inpcomm.getCStr();
    if ( *ptr == '@' )
	ptr++;

    ptr = parseNextWord( ptr, pnm );

    mSkipBlanks( ptr );
    if ( *ptr == '@' ) ptr++;
    BufferString tmp( ptr );
    pnm = tmp;

    if ( pnm.isEmpty() )
	return false;

    BufferString arg;
    while ( ptr && *ptr )
    {
	ptr = parseNextWord( ptr, arg );
	if ( !arg.isEmpty() )
	    args.add( arg );
    }

    mc.setProgram( pnm.buf() );
    mc.addArgs( args );

    return !mc.isBad();
}


bool GMTCommand::doExecute( od_ostream& strm, const char* fnm )
{
    strm << "Executing custom command" << od_endl;
    const char* res = find( ODGMT::sKeyCustomComm() );
    if ( !res || !*res )
	mErrStrmRet("No command to execute")

    strm << res << od_endl;
    OS::MachineCommand mc;
    setFromSingleStringRep( res, mc );
    BufferStringSet& args = const_cast<BufferStringSet&>( mc.args() );
    bool haspsfile = false;
    for ( int idx=0; idx<args.size(); idx++ )
    {
	BufferString* arg = args[idx];
	if ( arg->startsWith("-R") )
	{
	    mGetRangeString( *arg )
	    arg->insertAt( 0, "-R" );
	}
	else if ( arg->startsWith("-J") )
	{
	    mGetProjString( *arg, "X" )
	    arg->insertAt( 0, "-J" );
	}
	else if ( arg->contains(".ps") )
	    haspsfile = true;
    }

    if ( haspsfile )
    {
	bool getfileparts = false;
	BufferStringSet filenmparts;
	for ( int idx=0; idx<args.size(); idx++ )
	{
	    const BufferString& arg = args.get(idx);
	    if ( !getfileparts && arg.matches(">>") )
	    {
		getfileparts = true;
		continue;
	    }
	    else if ( getfileparts )
		filenmparts.add( arg.str() );
	}

	if ( getfileparts )
	{
	    args.removeRange( args.size()-filenmparts.size(), args.size()-1 );
	    mc.addArg( "-O" ).addArg( "-K" )
	      .addArg( BufferString(filenmparts.cat(" ") ) );
	}
    }

    if ( !execCmd(mc,strm) )
	mErrStrmRet("... Failed")

    strm << "... Done" << od_endl;
    return true;
}

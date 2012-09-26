/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "bufstringset.h"
#include "color.h"
#include "draw.h"
#include "filepath.h"
#include "gmtbasemap.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include <iostream>


static const int cTitleBoxHeight = 4;
static const int cTitleBoxWidth = 8;

int GMTBaseMap::factoryid_ = -1;

void GMTBaseMap::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Basemap", GMTBaseMap::createInstance );
}

GMTPar* GMTBaseMap::createInstance( const IOPar& iop )
{
    return new GMTBaseMap( iop );
}


bool GMTBaseMap::execute( std::ostream& strm, const char* fnm )
{
    strm << "Creating the Basemap ...  ";
    FixedString maptitle = find( ODGMT::sKeyMapTitle() );
    Interval<float> lblintv;
    if ( !get(ODGMT::sKeyLabelIntv(),lblintv) )
	mErrStrmRet("Incomplete data for basemap creation")

    bool closeps = false, dogrid = false;
    getYN( ODGMT::sKeyClosePS(), closeps );
    getYN( ODGMT::sKeyDrawGridLines(), dogrid );

    BufferString comm = "psbasemap ";
    BufferString rangestr; mGetRangeProjString( rangestr, "X" );
    comm += rangestr; comm += " -Ba";
    comm += lblintv.start;
    if ( dogrid ) { comm += "g"; comm += lblintv.start; }
    comm += "/a"; comm += lblintv.stop;
    if ( dogrid ) { comm += "g"; comm += lblintv.stop; }
    comm += ":\"."; comm += maptitle; comm += "\":";
    comm += " --Y_AXIS_TYPE=ver_text";
    comm += " --HEADER_FONT_SIZE=24";
    get( ODGMT::sKeyMapDim(), mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    comm += " --X_ORIGIN="; comm += xmargin;
    comm += "c --Y_ORIGIN="; comm += ymargin;
    comm += "c --PAPER_MEDIA=Custom_";
    float pagewidth = mapdim.start + 5 * xmargin;
    const float pageheight = mapdim.stop + 3 * ymargin;
    comm += pageheight < 21 ? 21 : pageheight; comm += "cx";
    comm += pagewidth < 21 ? 21 : pagewidth; comm += "c -K ";

    comm += "1> "; comm += fileName( fnm );
    if ( !execCmd(comm,strm) )
	mErrStrmRet("Failed to create Basemap")

    strm << "Done" << std::endl;

    strm << "Posting title box ...  ";
    comm = "@pslegend "; comm += rangestr; comm += " -F -O -Dx";
    comm += mapdim.start + xmargin; comm += "c/";
    comm += 0; comm += "c/";
    comm += cTitleBoxWidth; comm += "c/";
    comm += cTitleBoxHeight; comm += "c/BL ";
    if ( !closeps ) comm += "-K ";

    comm += "-UBL/0/0 ";    
    comm += "1>> "; comm += fileName( fnm );
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay title box")

    *sd.ostrm << "H 16 4 " << maptitle << std::endl;
    *sd.ostrm << "G 0.5l" << std::endl;
    int scaleval = 1;
    get( ODGMT::sKeyMapScale(), scaleval );
    *sd.ostrm << "L 10 4 C Scale  1:" << scaleval << std::endl;
    *sd.ostrm << "D 0 1p" << std::endl;
    BufferStringSet remset;
    get( ODGMT::sKeyRemarks(), remset );
    for ( int idx=0; idx<remset.size(); idx++ )
	*sd.ostrm << "L 12 4 C " << remset.get(idx) << std::endl;

    *sd.ostrm << std::endl;
    sd.close();
    strm << "Done" << std::endl;
    return true;
}


int GMTLegend::factoryid_ = -1;

void GMTLegend::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Legend", GMTLegend::createInstance );
}

GMTPar* GMTLegend::createInstance( const IOPar& iop )
{
    return new GMTLegend( iop );
}


bool GMTLegend::execute( std::ostream& strm, const char* fnm )
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

    BufferString rangestr; mGetRangeProjString( rangestr, "X" );
    get( ODGMT::sKeyMapDim(), mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    if ( parwithcolorbar >= 0 )
    {
	hascolbar = true;
	const IOPar* par = parset[parwithcolorbar];
	StepInterval<float> rg;
	par->get( ODGMT::sKeyDataRange(), rg );
	FilePath fp( fnm );
	fp.setExtension( "cpt" );
	BufferString colbarcomm = "psscale --LABEL_FONT_SIZE=12 ";
	colbarcomm += "--ANNOT_FONT_SIZE_PRIMARY=10 -D";
	colbarcomm += mapdim.start + xmargin; colbarcomm += "c/";
	colbarcomm += 1.2 * ymargin + cTitleBoxHeight; colbarcomm += "c/";
	colbarcomm += 2 * ymargin; colbarcomm += "c/";
	colbarcomm += xmargin / 2; colbarcomm += "c -O -C";
	colbarcomm += fileName( fp.fullPath() ); colbarcomm += " -B";
	colbarcomm += rg.step * 5; colbarcomm += ":\"";
	colbarcomm += par->find( sKey::Name() ); colbarcomm += "\":/:";
	colbarcomm += par->find( ODGMT::sKeyAttribName() );
	colbarcomm += ": -K 1>> "; colbarcomm += fileName( fnm );
	if ( !execCmd(colbarcomm,strm) )
	    mErrStrmRet("Failed to post color bar")

	StreamProvider( fp.fullPath() ).remove();
    }

    const int nritems = parset.size();
    BufferString comm = "@pslegend ";
    comm += rangestr; comm += " -O -Dx";
    comm += mapdim.start + xmargin; comm += "c/";
    comm += ymargin / 2 + cTitleBoxHeight + ( hascolbar ? 2 * ymargin : 0 );
    comm += "c/"; comm += 10; comm += "c/";
    comm += nritems ? nritems : 1; comm += "c/BL ";
    
    comm += "1>> "; comm += fileName( fnm );
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay legend")

    for ( int idx=0; idx<nritems; idx++ )
    {
	IOPar* par = parset[idx];
	FixedString namestr = par->find( sKey::Name() );
	if ( namestr.isEmpty() )
	    continue;

	float sz = 1;
	BufferString symbstr, penstr;
	bool usewellsymbol = false;
	par->getYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbol );
	FixedString shapestr = par->find( ODGMT::sKeyShape() );
	if ( !usewellsymbol && !shapestr ) continue;
	ODGMT::Shape shape = ODGMT::parseEnumShape( shapestr.str() );
	symbstr = ODGMT::sShapeKeys()[(int)shape];
	par->get( sKey::Size(), sz );
	if ( shape == ODGMT::Polygon || shape == ODGMT::Line )
	{
	    const char* lsstr = par->find( ODGMT::sKeyLineStyle() );
	    if ( !lsstr ) continue;

	    LineStyle ls;
	    ls.fromString( lsstr );
	    if ( ls.type_ != LineStyle::None )
	    {
		mGetLineStyleString( ls, penstr );
	    }
	    else if ( shape == ODGMT::Line )
		continue;
	}
	else
	{
	    Color pencol;
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
	    Color fillcol;
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
	*sd.ostrm << legendstring << std::endl;
	*sd.ostrm << "G0.2c" << std::endl;
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}



int GMTCommand::factoryid_ = -1;

void GMTCommand::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Advanced", GMTCommand::createInstance );
}

GMTPar* GMTCommand::createInstance( const IOPar& iop )
{
    return new GMTCommand( iop );
}


const char* GMTCommand::userRef() const
{
    BufferString* str = new BufferString( "GMT Command: " );
    const char* res = find( ODGMT::sKeyCustomComm() );
    *str += res;
    *( str->buf() + 25 ) = '\0';
    return str->buf();
}


bool GMTCommand::execute( std::ostream& strm, const char* fnm )
{
    strm << "Executing custom command" << std::endl;
    const char* res = find( ODGMT::sKeyCustomComm() );
    if ( !res || !*res )
	mErrStrmRet("No command to execute")

    strm << res << std::endl;
    BufferString comm = res;
    char* commptr = comm.buf();
    char* ptr = strstr( commptr, " -R" );
    if ( ptr )
    {
	BufferString oldstr( ptr );
	ptr = oldstr.buf();
	ptr++;
	while ( ptr && *ptr != ' ' )
	    ptr++;

	*ptr = '\0';
	BufferString newstr;
	mGetRangeString( newstr )
	newstr.insertAt( 0, " " );
	replaceString( commptr, oldstr.buf(), newstr.buf() );
    }

    ptr = strstr( commptr, " -J" );
    if ( ptr )
    {
	BufferString oldstr( ptr );
	ptr = oldstr.buf();
	ptr++;
	while ( ptr && *ptr != ' ' )
	    ptr++;

	*ptr = '\0';
	BufferString newstr;
	mGetProjString( newstr, "X" )
	newstr.insertAt( 0, " " );
	replaceString( commptr, oldstr.buf(), newstr.buf() );
    }

    ptr = strstr( commptr, ".ps" );
    if ( ptr )
    {
	ptr = strstr( commptr, ">>" );
	if ( ptr )
	{
	    BufferString temp = ptr;
	    *ptr = '\0';
	    comm += " -O -K ";
	    comm += temp;
	}
    }

    if ( !execCmd(comm.buf(),strm) )
	mErrStrmRet("... Failed")

    strm << "... Done" << std::endl;
    return true;
}


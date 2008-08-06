/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: gmtbasemap.cc,v 1.2 2008-08-06 09:58:20 cvsraman Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "draw.h"
#include "gmtbasemap.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"

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
    BufferString maptitle = find( ODGMT::sKeyMapTitle );
    Interval<float> xrg, yrg, mapdim, lblintv;
    if ( !get(ODGMT::sKeyXRange,xrg) || !get(ODGMT::sKeyYRange,yrg)
      || !get(ODGMT::sKeyMapDim,mapdim) || !get(ODGMT::sKeyLabelIntv,lblintv) )
	mErrStrmRet("Incomplete data for basemap creation")

    bool closeps;
    getYN( ODGMT::sKeyClosePS, closeps );

    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    const float pagewidth = mapdim.start + 4 * xmargin;
    const float pageheight = mapdim.stop + 3 * ymargin;
    BufferString comm = "psbasemap -R";
    comm += xrg.start; comm += "/"; comm += xrg.stop; comm += "/";
    comm += yrg.start; comm += "/"; comm += yrg.stop; comm += " -JX";
    comm += mapdim.start; comm += "c/"; comm += mapdim.stop; comm += "c -B";
    comm += lblintv.start; comm += "/"; comm += lblintv.stop;
    comm += ":\"."; comm += maptitle; comm += "\":";
    comm += " --X_ORIGIN="; comm += xmargin;
    comm += "c --Y_ORIGIN="; comm += ymargin;
    comm += "c --PAPER_MEDIA=Custom_";
    comm += pageheight; comm += "cx"; comm += pagewidth; comm += "c ";
    if ( !closeps ) comm += "-K ";
    comm += "> "; comm += fnm;
    if ( system(comm) )
	mErrStrmRet("Failed to create Basemap")

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
    ObjectSet<IOPar> parset;
    for ( int idx=0; idx<100; idx++ )
    {
	IOPar* par = subselect( idx );
	if ( !par ) break;

	parset += par;
    }

    if ( !parset.size() )
	mErrStrmRet("No legends to post")

    BufferString comm = "@pslegend -R1/10/1/10 -J -O -D12/1/8c/";
    comm += parset.size(); comm += "c/BL ";
    
    comm += ">> "; comm += fnm;
    StreamData sd = StreamProvider(comm).makeOStream();
    if ( !sd.usable() ) mErrStrmRet("Failed to overlay legend")

    for ( int idx=0; idx<parset.size(); idx++ )
    {
	IOPar* par = parset[idx];
	BufferString namestr = par->find( sKey::Name );
	if ( namestr.isEmpty() )
	    continue;

	float size = 0.5;
	BufferString symbstr, penstr;
	const char* shapestr = par->find( ODGMT::sKeyShape );
	if ( shapestr )
	{
	    const int shape = eEnum( ODGMT::Shape, shapestr );
	    if ( shape < 0 ) continue;

	    symbstr = ODGMT::sShapeKeys[shape];
	    par->get( sKey::Size, size );
	    Color pencol;
	    par->get( sKey::Color, pencol );
	    BufferString colstr;
	    mGetColorString( pencol, colstr );
	    penstr = "1p,"; penstr += colstr;
	}
	else
	{
	    const char* lsstr = par->find( ODGMT::sKeyLineStyle );
	    if ( !lsstr ) continue;

	    symbstr = "-";
	    LineStyle ls;
	    ls.fromString( lsstr );
	    symbstr = "n";
	    mGetLineStyleString( ls, penstr );
	}

	bool dofill;
	par->getYN( ODGMT::sKeyFill, dofill );
	BufferString legendstring = "S 0.6c ";
	legendstring += symbstr; legendstring += " "; 
	legendstring += size > 0.5 ? 0.5 : size;
	legendstring += "c ";
	if ( dofill )
	{
	    BufferString fillcolstr;
	    Color fillcol;
	    par->get( ODGMT::sKeyFillColor, fillcol );
	    mGetColorString( fillcol, fillcolstr );
	    legendstring += fillcolstr;
	}
	else
	    legendstring += "-";

	legendstring += " "; legendstring += penstr;
	legendstring += " "; legendstring += 1.3;
	legendstring += " "; legendstring += namestr;
	*sd.ostrm << legendstring << std::endl;
    }

    sd.close();
    strm << "Done" << std::endl;
    return true;
}


/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksreader.h"
#include "seisselection.h"
#include "seistrc.h"
#include "uistrings.h"
#include "scaler.h"
#include "datachar.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "posinfo.h"
#include "survgeom3d.h"
#include "separstr.h"
#include "od_istream.h"
#include "ascstream.h"
#include "zdomain.h"


Seis::Blocks::Reader::Reader( const char* inp )
    : survgeom_(0)
    , cubedata_(*new PosInfo::CubeData)
{
    File::Path fp( inp );
    if ( !File::exists(inp) )
    {
	if ( !inp || !*inp )
	    state_.set( tr("No input specified") );
	else
	    state_.set( uiStrings::phrDoesntExist(toUiString(inp)) );
	return;
    }

    if ( !File::isDirectory(inp) )
	fp.setExtension( 0 );
    filenamebase_ = fp.fileName();
    fp.setFileName( 0 );
    basepath_ = fp;

    readMainFile();
}


Seis::Blocks::Reader::~Reader()
{
    delete seldata_;
    delete survgeom_;
    delete &cubedata_;
}


void Seis::Blocks::Reader::readMainFile()
{
    const BufferString fnm( mainFileName() );
    od_istream strm( mainFileName() );
    if ( !strm.isOK() )
    {
	state_.set( uiStrings::phrCannotOpen(toUiString(strm.fileName())) );
	strm.addErrMsgTo( state_ );
	return;
    }

    ascistream astrm( strm );
    if ( !astrm.isOfFileType(sKeyFileType()) )
    {
	state_.set( tr("%1\nhas wrong file type").arg(strm.fileName()) );
	return;
    }

    bool havegensection = false, havepossection = false;
    while ( !havepossection )
    {
	if ( atEndOfSection(astrm) )
	    astrm.next();
	BufferString sectnm;
	if ( !strm.getLine(sectnm) )
	    break;

	if ( !sectnm.startsWith(sKeySectionPre()) )
	{
	    state_.set( tr("%1\n'%2' keyword not found")
		    .arg(strm.fileName()).arg(sKeySectionPre()) );
	    return;
	}

	bool failed = false;
	if ( sectnm == sKeyPosSection() )
	{
	    failed = !cubedata_.read( strm, true );
	    havepossection = true;
	}
	else if ( sectnm == sKeyGenSection() )
	{
	    IOPar iop;
	    iop.getFrom( astrm );
	    failed = !getGeneralSectionData( iop );
	    havegensection = true;
	}
	else
	{
	    IOPar* iop = new IOPar;
	    iop->getFrom( astrm );
	    iop->setName( sectnm.str() + FixedString(sKeySectionPre()).size() );
	    auxiops_ += iop;
	}
	if ( failed )
	{
	    state_.set( tr("%1\n'%2' section is invalid")
		    .arg(strm.fileName()).arg(sectnm) );
	    return;
	}
    }

    if ( !havegensection || !havepossection )
    {
	state_.set( tr("%1\nlacks %1 section").arg(strm.fileName())
	       .arg( havegensection ? tr("Position") : tr("General") ) );
	return;
    }
}


bool Seis::Blocks::Reader::getGeneralSectionData( const IOPar& iop )
{
    int ver = version_;
    iop.get( sKeyFmtVersion(), ver );
    version_ = (unsigned short)ver;
    iop.get( sKeyCubeName(), cubename_ );
    if ( cubename_.isEmpty() )
	cubename_ = filenamebase_;

    survgeom_ = new SurvGeom( cubename_, ZDomain::SI() );
    survgeom_->getStructure( iop );
    DataCharacteristics::getUserTypeFromPar( iop, fprep_ );
    Scaler* scl = Scaler::get( iop );
    mDynamicCast( LinScaler*, scaler_, scl );

    return true;
}


void Seis::Blocks::Reader::setSelData( const SelData* sd )
{
    if ( seldata_ != sd )
    {
	delete seldata_;
	if ( sd )
	    seldata_ = sd->clone();
	else
	    seldata_ = 0;
	needreset_ = true;
    }
}

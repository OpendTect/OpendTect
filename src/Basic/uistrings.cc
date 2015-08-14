/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		25/08/1999
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uistrings.h"

static const char* joinstring = "%1 %2";

#define mImmediateImpl( txt ) return phrImmediate( immediate, txt );

uiString uiStrings::phrImmediate( bool immediate, const uiString& string )
{ return immediate ? string : uiString( "%1 ..." ).arg( string ); }

uiString uiStrings::phrSelect( const uiString& string )
{ return uiString(joinstring).arg( sSelect(true) ).arg( string ); }

uiString uiStrings::phrDoesntExist(const uiString& string, int num )
{ return tr( "%1 does not exist", 0, num ); }

uiString uiStrings::phrExport( const uiString& string )
{ return uiString(joinstring).arg( sExport() ).arg( string ); }

uiString uiStrings::phrImport( const uiString& string )
{ return uiString(joinstring).arg( sImport() ).arg( string ); }

uiString uiStrings::phrCannotCreate( const uiString& string )
{ return tr("Cannot create %1").arg( string ); }

uiString uiStrings::phrCannotFind( const uiString& string )
{ return tr("Cannot find %1").arg( string ); }

uiString uiStrings::phrCannotOpen( const uiString& string )
{ return tr("Cannot open %1").arg( string ); }

uiString uiStrings::phrCannotRead( const uiString& string )
{ return tr("Cannot read %1").arg( string ); }

uiString uiStrings::phrCannotWrite( const uiString& string )
{ return tr("Cannot write %1").arg( string ); }

uiString uiStrings::phrCreate( const uiString& string )
{ return uiString(joinstring).arg( sCreate(true) ).arg( string ); }

uiString uiStrings::s2D( bool immediate )
{ mImmediateImpl( tr("2D") ); }

uiString uiStrings::s3D( bool immediate )
{ mImmediateImpl( tr("3D") ); }

uiString uiStrings::sAdd( bool immediate )
{ mImmediateImpl( tr("Add") ); }

uiString uiStrings::sASCII( bool immediate )
{ mImmediateImpl( tr("ASCII") ); }

uiString uiStrings::sAttributes( bool immediate )
{ mImmediateImpl( tr("Attributes") ); }

uiString uiStrings::sColorTable( bool immediate )
{ mImmediateImpl( tr("ColorTable") ); }

uiString uiStrings::sCreate( bool immediate )
{ mImmediateImpl( tr("Create") ); }

uiString uiStrings::sCantCreateHor()
{ return phrCannotCreate( tr("horizon") ); }

uiString uiStrings::sCantFindAttrName()
{ return phrCannotFind( tr("attribute name") ); }

uiString uiStrings::sCantFindODB()
{ return phrCannotFind( tr("object in data base") ); }

uiString uiStrings::sCantFindSurf()
{ return phrCannotFind( tr("surface") ); }

uiString uiStrings::sCantReadHor()
{ return phrCannotRead( tr("horizon") ); }

uiString uiStrings::sCantReadInp()
{ return phrCannotRead( tr("input") ); }

uiString uiStrings::sCantWriteSettings()
{ return phrCannotWrite(tr("settings"));}

uiString uiStrings::sCantOpenInpFile( int num )
{ return phrCannotOpen( tr("input file", 0, num ) ); }

uiString uiStrings::sCantOpenOutpFile( int num )
{ return phrCannotOpen( tr("output file", 0, num ) ); }

uiString uiStrings::sCreateProbDesFunc()
{ return phrCreate( sProbDensFunc() ); }

uiString uiStrings::sCreateRandLines()
{ return phrCreate( sRandomLine() ); }

uiString uiStrings::sEdit( bool immediate )
{ mImmediateImpl( tr("Edit") ); }

uiString uiStrings::sFaults( bool immediate, int num )
{ mImmediateImpl( tr("Faults", 0, num ) ); }

uiString uiStrings::sFaultStickSets( int num )
{ return tr( "Fault Stick Set", 0, num ); }

uiString uiStrings::sHelp( bool immediate )
{ mImmediateImpl( tr("Help") ); }

uiString uiStrings::sHistogram( bool immediate )
{ mImmediateImpl( tr("Histogram") ); }

uiString uiStrings::sHorizons( bool immediate, int num )
{ mImmediateImpl( tr("Horizons", 0, num ) ); }

uiString uiStrings::sLoad( bool immediate )
{ mImmediateImpl( tr("Load") ); }

uiString uiStrings::sLogs( bool immediate )
{ mImmediateImpl( tr("Logs") ); }

uiString uiStrings::sMarkers( bool immediate, int num )
{ mImmediateImpl( tr("Markers", 0, num) ); }

uiString uiStrings::sNew( bool immediate )
{ mImmediateImpl( tr("New") ); }

uiString uiStrings::sOpen( bool immediate )
{ mImmediateImpl( tr("Open") ); }

uiString uiStrings::sOptions( bool immediate )
{ mImmediateImpl( tr("Options") ); }

uiString uiStrings::sProbDensFunc()
{ return tr("Probability Density Function"); }

uiString uiStrings::sProperties( bool immediate )
{ mImmediateImpl( tr("Properties") ); }

uiString uiStrings::sRemove( bool immediate )
{ mImmediateImpl( tr("Remove") ); }

uiString uiStrings::sSave( bool immediate )
{ mImmediateImpl( tr("Save") ); }

uiString uiStrings::sSaveAs( bool immediate )
{ mImmediateImpl( tr("Save as") ); }

uiString uiStrings::sSeismic( bool immediate, int num )
{ mImmediateImpl( tr("Seismic", 0, num ) ); }

uiString uiStrings::sSelect(bool immediate)
{ mImmediateImpl( tr("Select") ); }

uiString uiStrings::sSetting( bool immediate, int num )
{ mImmediateImpl( tr("Setting", 0, num) ); }

uiString uiStrings::sShift(bool immediate)
{ mImmediateImpl( tr("Shift") ); }

uiString uiStrings::sStored( bool immediate )
{ mImmediateImpl( tr("Stored") ); }

uiString uiStrings::sStratigraphy( bool immediate )
{ mImmediateImpl( tr("Stratigraphy") ); }

uiString uiStrings::sTrack( bool immediate )
{ mImmediateImpl( tr("Track") ); }

uiString uiStrings::sWell( bool immediate, int num )
{ mImmediateImpl( tr("Well", 0, num ) ); }


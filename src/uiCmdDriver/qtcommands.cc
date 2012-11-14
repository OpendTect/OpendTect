/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          February 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "qtcommands.h"

#include "cmddriverbasics.h"
#include "cmdrecorder.h"

#include "file.h"

#include "uicolor.h"
#include "uifiledlg.h"
#include "uimainwin.h"


namespace CmdDrive
{


SetColorActivator::SetColorActivator(const Color& col)
    : color_(col)
{}


void SetColorActivator::actCB( CallBacker* )
{ setExternalColor( color_ ); }


#define mParChannel( channelnm, parstr, parnext, val, optional ) \
\
    const char* parnext; \
    double num##val = strtod( parstr, const_cast<char**>(&parnext) ); \
    if ( parnext!=parstr && *parnext && !isspace(*parnext) ) \
        parnext = parstr; \
\
    if ( (parnext==parstr && !(optional)) || num##val<0 || num##val>255 ) \
    { \
	mParseErrStrm << channelnm << "-channel requires single number " \
		      << "between 0 and 255" << std::endl; \
	return false; \
    } \
    const unsigned char val( mCast(unsigned char,int(num##val+0.5)) );


bool ColorOkCmd::act( const char* parstr )
{
    if ( uiMainWin::activeModalType() != uiMainWin::Colour ) 
    { 
	mWinErrStrm << "Command requires open QColorDialog" << std::endl; 
	return false; 
    } 

    mParDQuoted( "color string", parstr, parextra, colorstr, true, true );
    if ( parstr != parextra )
    {
	FileMultiString fms( colorstr );
	if ( fms.size()!=3 && fms.size()!=4 )
	{
	    mParseErrStrm << "Color string must contain 3 or 4 channels"
			  << std::endl;
	    return false;
	}
	if ( strchr(fms.buf(),' ') || strchr(fms.buf(),'\t') )
	{
	    mParseErrStrm << "Color string should not contain white space"
			  << std::endl;
	    return false;
	}
	fms.setSepChar( ' ' );

	colorstr = fms.unescapedStr();
	colorstr += " "; colorstr += parextra;
	parstr = colorstr;
    }

    mParChannel( "Red", parstr, parnext, red, parextra==parstr );
    mParChannel( "Green", parnext, parnexxt, green, parnext==parstr );
    mParChannel( "Blue", parnexxt, parnexxxt, blue, parnext==parstr );

    Color col( red, green, blue );

    BufferString colorword;
    if ( parnext == parstr )
    {
	const char* partemp = getNextWord( parstr, colorword.buf() );
	parnexxxt = const_cast<char*>( partemp );
	if ( colorword.isEmpty() )
	{
	    mParseErrStrm << "Need color tag or RGB-values to specify color"
			  << std::endl;
	    return false;
	}

	if ( mMatchCI(colorword, "Black") )
	    col.set( 0, 0, 0 ); 
	else if ( mMatchCI(colorword, "Blue") )
	    col.set( 0, 0, 255 );
	else if ( mMatchCI(colorword, "Brown") )
	    col.set( 170, 85, 0 );
	else if ( mMatchCI(colorword, "Cyan") )
	    col.set( 0, 255, 255 );
	else if ( mMatchCI(colorword, "Green") )
	    col.set( 0, 127, 0 );
	else if ( mMatchCI(colorword, "Grey") )
	    col.set( 170, 170, 170 );
	else if ( mMatchCI(colorword, "Lilac") )
	    col.set( 170, 85, 255 );
	else if ( mMatchCI(colorword, "Lime") )
	    col.set( 0, 255, 0 );
	else if ( mMatchCI(colorword, "Magenta") )
	    col.set( 255, 0, 255 );
	else if ( mMatchCI(colorword, "Olive") )
	    col.set( 127, 127, 0 );
	else if ( mMatchCI(colorword, "Orange") )
	    col.set( 255, 170, 0 );
	else if ( mMatchCI(colorword, "Purple") )
	    col.set( 127, 0, 127 );
	else if ( mMatchCI(colorword, "Pink") )
	    col.set( 255, 170, 255 );
	else if ( mMatchCI(colorword, "Red") )
	    col.set( 255, 0, 0 );
	else if ( mMatchCI(colorword, "White") )
	    col.set( 255, 255, 255 );
	else if ( mMatchCI(colorword, "Yellow") )
	    col.set( 255, 255, 0 );
	else 
	{
	    mParseErrStrm << "Color tag not in {Black, Blue, Brown, Cyan, "
		          << "Green, Grey, Lilac, Lime, Magenta, Olive, "
			  << "Orange, Purple, Pink, Red, White, Yellow]"
			  << std::endl;
	    return false; 
	}
    }

    mParChannel( "Transparency", parnexxxt, partail, transparency, true );
    col.setTransparency( transparency );
    mParTail( partail );

    mActivate( SetColor, Activator(col) );
    mActivate( CloseQDlg, Activator(1) );
    return true;
}


#define mCheckExternalFilenamesErrMsg( warningsallowed ) \
{ \
    const char* errmsg = uiFileDialog::getExternalFilenamesErrMsg(); \
    if ( errmsg ) \
    { \
	bool warn = false; \
	if ( *errmsg == '!' ) \
	{ \
	    errmsg++; \
	    warn = warningsallowed; \
	} \
	if ( !warn ) \
	{ \
	    mWinErrStrm << errmsg << std::endl; \
	    return false; \
	} \
	mWinWarnStrm << errmsg << std::endl; \
    } \
}

bool FileOkCmd::act( const char* parstr )
{
    if ( uiMainWin::activeModalType() != uiMainWin::File ) 
    { 
	mWinErrStrm << "Command requires open QFileDialog" << std::endl; 
	return false; 
    } 
    
    mParDQuoted( "file path set", parstr, partail, fpsetstr, false, false ); 
    StringProcessor(fpsetstr).makeDirSepIndep();
    mGetEscConvertedFMS( fms, fpsetstr, true ); 
    mParTail( partail );

    uiFileDialog::setExternalFilenames( fms );
    mActivate( CloseQDlg, Activator(0) );
    mCheckExternalFilenamesErrMsg( true );
    return true;	
}


bool SnapshotCmd::act( const char* parstr )
{
    mParDQuoted( "file path", parstr, parnext, fpstr, false, true ); 
    StringProcessor(fpstr).makeDirSepIndep();
    mGetEscConvertedFMS( fms, fpstr, true ); 
    
    int zoom = openQDlg() ? 2 : 1;
    const uiMainWin* grabwin = curWin();

    BufferString zoomtag; 
    const char* partail = getNextWord( parnext, zoomtag.buf() ); 

    if ( mMatchCI(zoomtag,"ODMain") && applWin() ) 
    {
	grabwin = applWin();
	zoom = 1; 
    }
    else if ( mMatchCI(zoomtag,"Desktop") ) 
	zoom = 0;
    else if ( !mMatchCI(zoomtag,"CurWin") ) 
	partail = parnext;

    mParTail( partail ); 

    const char* writeexts = "*.bmp;;*.jpg;;*.jpeg;;*.png;;*.ppm;;*.xbm;;*.xpm";
    uiFileDialog tmpfiledlg( 0, false, 0, writeexts );
    if ( File::isDirectory(outputDir()) )
	tmpfiledlg.setDirectory( outputDir() );
    uiFileDialog::setExternalFilenames( fms );
    tmpfiledlg.processExternalFilenames();
    mCheckExternalFilenamesErrMsg( false );

    mActivate( Snapshot, Activator(*grabwin, tmpfiledlg.fileName(), zoom) );
    return true;
}


SnapshotActivator::SnapshotActivator( const uiMainWin& grabwin,
				      const char* filenm, int zoom )
    : actgrabwin_( const_cast<uiMainWin&>(grabwin) )
    , actfilenm_( filenm )
    , actzoom_( zoom )
{}


void SnapshotActivator::actCB( CallBacker* cb )
{ actgrabwin_.grab( actfilenm_, actzoom_ ); }


//====== CmdComposers =========================================================


bool QColorDlgCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ev.begin_ )
	return true;

    BufferString qcolordlgword;
    const char* msgnext = getNextWord( ev.msg_, qcolordlgword.buf() );
    char* msgnexxt;
    char* msgtail;

    const int red   = strtol( msgnext,  &msgnexxt, 0 );
    const int green = strtol( msgnexxt, &msgnexxt, 0 );
    const int blue  = strtol( msgnexxt, &msgnexxt, 0 );
    const int transparency = strtol( msgnexxt, &msgtail,  0 );

    insertWindowCaseExec( ev );
    if ( msgnext == msgnexxt )
    {
	mRecOutStrm << "Cancel" << std::endl;
    }
    else if ( msgnexxt == msgtail )
    {
	mRecOutStrm << "ColorOk " << red << " " << green << " " << blue
		    << std::endl;
    }
    else
	mRecOutStrm << "ColorOk " << red << " " << green << " " << blue
		    << " " << transparency <<  std::endl;

    return true;
}


bool QFileDlgCmdComposer::accept( const CmdRecEvent& ev )
{
    if ( !CmdComposer::accept(ev) )
	return false;

    if ( ev.begin_ )
	return true;

    BufferString qfiledlgword;
    const char* msgnext = getNextWord( ev.msg_, qfiledlgword.buf() );

    insertWindowCaseExec( ev );
    if ( *msgnext )
    {
	FileMultiString fms( msgnext+1 );
	FileMultiString dressedfms;
	for ( int idx=0; idx<fms.size(); idx++ )
	{
	    BufferString filepath = fms[idx];
	    mDressUserInputString( filepath, sFilePathSet );
	    dressedfms += filepath;
	}
	mRecOutStrm << "FileOk \"" << dressedfms.unescapedStr() << "\""
		    << std::endl;
    }
    else
	mRecOutStrm << "Cancel" << std::endl;

    return true;
}


}; // namespace CmdDrive

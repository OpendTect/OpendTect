#ifndef cmddriverbasics_h
#define cmddriverbasics_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          January 2009
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "separstr.h"
#include "bufstringset.h"
#include "mousecursor.h"

class uiMainWin;


namespace CmdDrive
{

#define mAddCharToBufStr( bufstr, chr ) \
{ char str[2]; str[0] = chr; str[1] = '\0'; bufstr += str; }

#define mSepStrSize(fms) ( fms.isEmpty() ? 1 : fms.size() )

#define mGetAmpFilteredStr( dest, src ) \
    BufferString dest( src ); \
    StringProcessor(dest).filterAmpersands();


#define mGetEscConvertedFMS( fms, str, removecmdfileescapes ) \
\
    BufferString fms##bufstr( str ); \
    StringProcessor( fms##bufstr ).addFileMultiStrEscapes(); \
    if ( removecmdfileescapes ) \
	StringProcessor( fms##bufstr ).removeCmdFileEscapes(); \
\
    FileMultiString fms( fms##bufstr.buf() );


#define mStripOuterBrackets( name, context ) \
    StringProcessor(name).stripOuterBrackets( \
				StringProcessor::context##VoidBeginBrackets(), \
				StringProcessor::context##VoidEndBrackets() )


#define mDressString( name, context, cleanup ) \
{ \
    if ( cleanup ) \
	StringProcessor(name).cleanUp(); \
    while ( mStripOuterBrackets(name, sDress) ); \
    StringProcessor(name).addCmdFileEscapes( \
				StringProcessor::context##EscSymbols() ); \
}

#define mDressNameString( name, context ) \
    mDressString( name, context, true );

#define mDressUserInputString( name, context ) \
    mDressString( name, context, false );


#define mIsSymbol(chr) ( !isalnum(chr) && !isspace(chr) )


#define mGetColorString( color, isvalid, colorstr ) \
\
    FileMultiString colorstr; \
    colorstr += isvalid ? (int) color.r() : 255; \
    colorstr += isvalid ? (int) color.g() : 255; \
    colorstr += isvalid ? (int) color.b() : 255; \
    colorstr += isvalid ? (int) color.t() : 255; \


#define mUnscope( identstr, unscopedstr ) \
    const char* unscopedstr = *identstr=='@' ? identstr+1 : identstr;


mClass StringProcessor	// Parsing toolkit 
{
public:
			StringProcessor(BufferString& bs)
			    : constptr_(bs.buf()), bufstr_(&bs)
			{}

			StringProcessor(SeparString& ss)
			    : constptr_(ss.buf()), bufstr_(&ss.rep())
			{}

			StringProcessor(const char* str)
			    : constptr_(str), bufstr_(0)
			{}

    const char*		parseDQuoted(BufferString& unquotedstr) const;
    int			consecutiveDQuoted(const char** remnant=0) const;
    int 		nrDQuotes() const;

    const char*		parseBracketed(BufferString& unbracketedstr, 
				       char begin='[',char end=']')  const;

    int			removeNumAppendix();
    bool 		removeTokenAppendix(char token);

    const char*		nextAction(BufferString&) const;
    const char*		findAssignment(const char* tokens="=") const; 

    const char*		parseIdentifier(BufferString&) const;
    bool		convertToInt(int* =0) const;
    bool		convertToDouble(double* =0) const;

    char /* missing */	preParseProcedure(FileMultiString& key,
	    				  FileMultiString& parameters) const;

    void		makeDirSepIndep(int startpos=-1);

    void		addCmdFileEscapes(const char* extraescapesymbols=0);
    bool		isEscapedSymbol(const char*) const;
    void		removeCmdFileEscapes();
    void		addFileMultiStrEscapes();

    const char*		getCharElement(int idx) const; 
    int			nrCharElements() const;
    void		appendCharElements(const char* tail);

    void		filterAmpersands();
    void		capitalize(bool yn=true);

    void		removeExtraSpace();
    void		removeTrailingDots();
    void		cleanUp();

    char		stripOuterBrackets(const char* beginsymbols,
					   const char* endsymbols);

    const char*		buf() const			{ return constptr_; }

    static const char*	sWinAssertEscSymbols()		{ return "$*#[]"; }
    static const char*	sWinNameEscSymbols()		{ return "$*#\""; }
    static const char*	sKeyStrEscSymbols()		{ return "$*`#\""; }
    static const char*	sMenuPathEscSymbols()		{ return "$*`#\""; }
    static const char*	sTreePathEscSymbols()		{ return "$*`#<\""; }
    static const char*	sItemNameEscSymbols()		{ return "$*#\""; }
    static const char*	sInputStrEscSymbols()		{ return "$\""; }
    static const char*	sFilePathSetEscSymbols()	{ return "$`\""; }
    static const char*	sAllEscSymbols()		{ return "$*`#[]<\""; }

    static const char*	sMatchVoidBeginBrackets()   { return ":`([{<>\"/"; }
    static const char*	sMatchVoidEndBrackets()	    { return ":')]}><\"\\"; }

    static const char*	sDressVoidBeginBrackets()	{ return ">/"; }
    static const char*	sDressVoidEndBrackets()		{ return "<\\"; }

protected:

    const char*		constptr_;
    BufferString*	bufstr_;
};


mGlobal const char* windowTitle(const uiMainWin* applwin,
				const uiMainWin* uimw,
				int aliasnr=0);
mGlobal bool isCmdDriverWindow(const uiMainWin*);
mGlobal const char* controllerTitle();


mClass WindowStack
{
public:
			WindowStack()			{}

    bool		moveToTop(const uiMainWin*);
    bool		moveFrameToTop(const uiMainWin*);

    const uiMainWin*	topWin(bool ignorecmddriverwins=false) const;
    void		synchronize();

protected:

    bool		moveToTopWithoutSync(const uiMainWin*);

    ObjectSet<const uiMainWin> winstack_;
};


mClass InteractSpec
{
public:
			InteractSpec(bool wait=true)
			    : wait_( wait )
			    , launchpad_( 0 )
			    , cursorshape_( MouseCursor::Forbidden )
			{}

    bool		wait_;
    const uiMainWin*	launchpad_;
    MouseCursor::Shape	cursorshape_;
    BufferString	dlgtitle_;
    BufferString	okbuttext_;
    BufferString	cancelbuttext_;
    BufferString	infotext_;
    BufferString	resumetext_;
};


}; // namespace CmdDrive

#endif


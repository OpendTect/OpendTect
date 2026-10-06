#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtmod.h"

#include "enums.h"
#include "stringview.h"

namespace ODGMT
{
    enum Shape		{ Star, Circle, Diamond, Square, Triangle, Cross,
   			  Polygon, Line };
    			mDeclareNameSpaceEnumUtils(GMT,Shape)
    enum Resolution	{ Full, High, Intermediate, Low, Crude };
			mDeclareNameSpaceEnumUtils(GMT,Resolution)
    enum Alignment	{ Above, Below, Left, Right };
			mDeclareNameSpaceEnumUtils(GMT,Alignment);
    enum ExecStatus	{ Success, FatalError, Failure };

    static const char* shapekeysbuf[] =
	{ "a", "c", "d", "s", "t", "x", "n", "-", nullptr };
    static const char* resolkeysbuf[] = { "f", "h", "i", "l", "c" };

    mGlobal(GMT) inline const char**	sShapeKeys() { return shapekeysbuf; }
    mGlobal(GMT) inline const char**	sResolKeys() { return resolkeysbuf; }

    inline StringView	sKeyAttribName()	{ return "Attribute name"; }
    inline StringView	sKeyClipOutside()	{ return "Clip outside"; }
    inline StringView	sKeyClosePS()		{ return "Close PostScript"; }
    inline StringView	sKeyColSeq()		{ return "Color sequence"; }
    inline StringView	sKeyCustomComm()	{ return "Custom command"; }
    inline StringView	sKeyDataRange()		{ return "Data range"; }
    inline StringView	sKeyDrawContour()	{ return "Draw contour"; }
    inline StringView	sKeyDrawGridLines()	{ return "Draw gridlines"; }
    inline StringView	sKeyDryFill()		{ return "Fill Dry"; }
    inline StringView	sKeyDryFillColor()	{ return "Fill Color Dry"; }
    inline StringView	sKeyFill()		{ return "Fill"; }
    inline StringView	sKeyFillColor()		{ return "Fill Color"; }
    inline StringView	sKeyFlipColTab()	{ return "Flip color table"; }
    inline StringView	sKeyFontSize()		{ return "Font size"; }
    inline StringView	sKeyGMT()		{ return "GMT"; }
    inline int		sKeyGMTSelKey()		{ return 808080; }
    inline StringView	sKeyGroupName()		{ return "Group Name"; }
    inline StringView	sKeyLabelAlignment()	{ return "Label alignment"; }
    inline StringView	sKeyLabelIntv()		{ return "Label Interval"; }
    inline StringView	sKeyLegendParams()	{ return "Legend Parameters"; }
    inline StringView	sKeyLineNames()		{ return "Line names"; }
    inline StringView	sKeyLineStyle()		{ return "Line Style"; }
    inline StringView	sKeyMapDim()		{ return "Map Dimension"; }
    inline StringView	sKeyMapScale()		{ return "Map scale"; }
    inline StringView	sKeyMapTitle()		{ return "Map Title"; }
    inline StringView	sKeyPostLabel()		{ return "Post label"; }
    inline StringView	sKeyPostColorBar()	{ return "Post Color bar"; }
    inline StringView	sKeyPostStart()		{ return "Post start"; }
    inline StringView	sKeyPostStop()		{ return "Post stop"; }
    inline StringView	sKeyPostTitleBox()	{ return "Post title box"; }
    inline StringView	sKeyPostTraceNrs()	{ return "Post Trace Nrs"; }
    inline StringView	sKeyRemarks()		{ return "Remarks"; }
    inline StringView	sKeyResolution()	{ return "Resolution"; }
    inline StringView	sKeyShape()		{ return "Shape"; }
    inline StringView	sKeySkipWarning()	{ return "Skip Warning"; }
    inline StringView	sKeyStartClipping()	{ return "Start Clipping"; }
    inline StringView	sKeyUTMZone()		{ return "UTM zone"; }
    inline StringView	sKeyWetFill()		{ return "Fill Wet"; }
    inline StringView	sKeyWetFillColor()	{ return "Fill Color Wet"; }
    inline StringView	sKeyWellNames()		{ return "Well names"; }
    inline StringView	sKeyXRange()		{ return "X Range"; }
    inline StringView	sKeyYRange()		{ return "Y Range"; }
    inline StringView	sKeyZVals()		{ return "Z values"; }
    inline StringView	sKeyFaultID()		{ return "FaultID"; }
    inline StringView	sKeyHorizonID()		{ return "HorizonID"; }
    inline StringView	sKeyZIntersectionYN()	{ return "ZIntersection"; }
    inline StringView	sKeyUseFaultColorYN()	{ return "Use Fault Color"; }
    inline StringView	sKeyFaultColor()	{ return "Fault Color"; }
    inline StringView	sKeyUseWellSymbolsYN()	{ return "Use Well Symbols"; }
    inline StringView	sKeyWellSymbolName()	{ return "Symbol Name"; }
};


mExpClass(GMT) GMTWellSymbol : public NamedObject
{
public:
    BufferString	iconfilenm_;
    BufferString	deffilenm_;

    bool		usePar(const IOPar&);

    static const char*	sKeyIconFileName();
    static const char*	sKeyDefFileName();
};


mExpClass(GMT) GMTWellSymbolRepository
{
public:
    			GMTWellSymbolRepository();
			~GMTWellSymbolRepository();

   int			size() const;
   const GMTWellSymbol*	get(int) const;
   const GMTWellSymbol*	get(const char*) const;

protected:

   void			init();

   ObjectSet<GMTWellSymbol>	symbols_;
};


mGlobal(GMT) const GMTWellSymbolRepository& GMTWSR();


#define mGetDefault( key, fn, var ) \
    Settings::fetch("GMT").fn(key,var); \


#define mSetDefault( key, fn, var ) \
    Settings::fetch("GMT").fn(key,var); \
    Settings::fetch("GMT").write();

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "gmtmod.h"
#include "settings.h"
#include "enums.h"

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

    static const char* shapekeysbuf[] = { "a", "c", "d", "s", "t", "x", "n", "-", 0 };
    static const char* resolkeysbuf[] = { "f", "h", "i", "l", "c" };

    mGlobal(GMT) inline const char**	sShapeKeys() { return shapekeysbuf; }
    mGlobal(GMT) inline const char**	sResolKeys() { return resolkeysbuf; }
			
    mGlobal(GMT) inline const char*	sKeyAttribName() { return "Attribute name"; }
    mGlobal(GMT) inline const char*	sKeyClipOutside() { return "Clip outside"; }
    mGlobal(GMT) inline const char*	sKeyClosePS() { return "Close PostScript"; }
    mGlobal(GMT) inline const char*	sKeyColSeq() { return "Color sequence"; }
    mGlobal(GMT) inline const char*	sKeyCustomComm() { return "Custom command"; }
    mGlobal(GMT) inline const char*	sKeyDataRange() { return "Data range"; }
    mGlobal(GMT) inline const char*	sKeyDrawContour() { return "Draw contour"; }
    mGlobal(GMT) inline const char*	sKeyDrawGridLines() { return "Draw gridlines"; }
    mGlobal(GMT) inline const char*	sKeyDryFill() { return "Fill Dry"; }
    mGlobal(GMT) inline const char*	sKeyDryFillColor() { return "Fill Color Dry"; }
    mGlobal(GMT) inline const char*	sKeyFill() { return "Fill"; }
    mGlobal(GMT) inline const char*	sKeyFillColor() { return "Fill Color"; }
    mGlobal(GMT) inline const char*	sKeyFlipColTab() { return "Flip color table"; }
    mGlobal(GMT) inline const char*	sKeyFontSize() { return "Font size"; }
    mGlobal(GMT) inline const char*	sKeyGMT() { return "GMT"; }
    mGlobal(GMT) inline const char*	sKeyGMTSelKey() { return "808080"; }
    mGlobal(GMT) inline const char*	sKeyGroupName() { return "Group Name"; }
    mGlobal(GMT) inline const char*	sKeyLabelAlignment() { return "Label alignment"; }
    mGlobal(GMT) inline const char*	sKeyLabelIntv() { return "Label Interval"; }
    mGlobal(GMT) inline const char*	sKeyLegendParams() { return "Legend Parameters"; }
    mGlobal(GMT) inline const char*	sKeyLineNames() { return "Line names"; }
    mGlobal(GMT) inline const char*	sKeyLineStyle() { return "Line Style"; }
    mGlobal(GMT) inline const char*	sKeyMapDim() { return "Map Dimension"; }
    mGlobal(GMT) inline const char*	sKeyMapScale() { return "Map scale"; }
    mGlobal(GMT) inline const char*	sKeyMapTitle() { return "Map Title"; }
    mGlobal(GMT) inline const char*	sKeyPostLabel() { return "Post label"; }
    mGlobal(GMT) inline const char*	sKeyPostColorBar() { return "Post Color bar"; }
    mGlobal(GMT) inline const char*	sKeyPostStart() { return "Post start"; }
    mGlobal(GMT) inline const char*  sKeyPostStop() { return "Post stop"; }
    mGlobal(GMT) inline const char*	sKeyPostTitleBox() { return "Post title box"; }
    mGlobal(GMT) inline const char*	sKeyPostTraceNrs() { return "Post Trace Nrs"; }
    mGlobal(GMT) inline const char*	sKeyRemarks() { return "Remarks"; }
    mGlobal(GMT) inline const char*	sKeyResolution() { return "Resolution"; }
    mGlobal(GMT) inline const char*	sKeyShape() { return "Shape"; }
    mGlobal(GMT) inline const char*	sKeySkipWarning() { return "Skip Warning"; }
    mGlobal(GMT) inline const char*	sKeyStartClipping() { return "Start Clipping"; }
    mGlobal(GMT) inline const char*	sKeyUTMZone() { return "UTM zone"; }
    mGlobal(GMT) inline const char*	sKeyWetFill() { return "Fill Wet"; }
    mGlobal(GMT) inline const char*	sKeyWetFillColor() { return "Fill Color Wet"; }
    mGlobal(GMT) inline const char*	sKeyWellNames() { return "Well names"; }
    mGlobal(GMT) inline const char*	sKeyXRange() { return "X Range"; }
    mGlobal(GMT) inline const char*	sKeyYRange() { return "Y Range"; }
    mGlobal(GMT) inline const char*	sKeyZVals() { return "Z values"; }
    mGlobal(GMT) inline const char*  sKeyFaultID() { return "FaultID"; }
    mGlobal(GMT) inline const char*  sKeyHorizonID() { return "HorizonID"; }
    mGlobal(GMT) inline const char*  sKeyZIntersectionYN() { return "ZIntersection"; }
    mGlobal(GMT) inline const char*  sKeyUseFaultColorYN() { return "Use Fault Color"; }
    mGlobal(GMT) inline const char*  sKeyFaultColor() { return "Fault Color"; }
    mGlobal(GMT) inline const char*  sKeyUseWellSymbolsYN() { return "Use Well Symbols"; }
    mGlobal(GMT) inline const char*  sKeyWellSymbolName() { return "Symbol Name"; }
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


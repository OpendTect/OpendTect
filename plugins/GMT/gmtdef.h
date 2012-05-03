#ifndef gmtdef_h
#define gmtdef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtdef.h,v 1.18 2012-05-03 09:06:19 cvskris Exp $
________________________________________________________________________

-*/

#include "settings.h"
#include "enums.h"

namespace ODGMT
{
    enum Shape		{ Star, Circle, Diamond, Square, Triangle, Cross,
   			  Polygon, Line };
    			DeclareNameSpaceEnumUtils(Shape)
    enum Resolution	{ Full, High, Intermediate, Low, Crude };
			DeclareNameSpaceEnumUtils(Resolution)
    enum Alignment	{ Above, Below, Left, Right };
			DeclareNameSpaceEnumUtils(Alignment);
    enum ExecStatus	{ Success, FatalError, Failure };

    mGlobal inline const char**	sShapeKeys()
    			{
			    static const char* buf[] = { "a", "c", "d", "s", "t", "x", "n", "-", 0 };
			    return buf;
			}
    mGlobal inline const char**	sResolKeys()
    			{
			    static const char* buf[] = { "f", "h", "i", "l", "c" };
			    return buf;
			}

    mGlobal inline const char*	sKeyAttribName() { return "Attribute name"; }
    mGlobal inline const char*	sKeyClipOutside() { return "Clip outside"; }
    mGlobal inline const char*	sKeyClosePS() { return "Close PostScript"; }
    mGlobal inline const char*	sKeyColSeq() { return "Color sequence"; }
    mGlobal inline const char*	sKeyCustomComm() { return "Custom command"; }
    mGlobal inline const char*	sKeyDataRange() { return "Data range"; }
    mGlobal inline const char*	sKeyDrawContour() { return "Draw contour"; }
    mGlobal inline const char*	sKeyDrawGridLines() { return "Draw gridlines"; }
    mGlobal inline const char*	sKeyDryFill() { return "Fill Dry"; }
    mGlobal inline const char*	sKeyDryFillColor() { return "Fill Color Dry"; }
    mGlobal inline const char*	sKeyFill() { return "Fill"; }
    mGlobal inline const char*	sKeyFillColor() { return "Fill Color"; }
    mGlobal inline const char*	sKeyFlipColTab() { return "Flip color table"; }
    mGlobal inline const char*	sKeyFontSize() { return "Font size"; }
    mGlobal inline const char*	sKeyGMT() { return "GMT"; }
    mGlobal inline const char*	sKeyGMTSelKey() { return "808080"; }
    mGlobal inline const char*	sKeyGroupName() { return "Group Name"; }
    mGlobal inline const char*	sKeyLabelAlignment() { return "Label alignment"; }
    mGlobal inline const char*	sKeyLabelIntv() { return "Label Interval"; }
    mGlobal inline const char*	sKeyLegendParams() { return "Legend Parameters"; }
    mGlobal inline const char*	sKeyLineNames() { return "Line names"; }
    mGlobal inline const char*	sKeyLineStyle() { return "Line Style"; }
    mGlobal inline const char*	sKeyMapDim() { return "Map Dimension"; }
    mGlobal inline const char*	sKeyMapScale() { return "Map scale"; }
    mGlobal inline const char*	sKeyMapTitle() { return "Map Title"; }
    mGlobal inline const char*	sKeyPostLabel() { return "Post label"; }
    mGlobal inline const char*	sKeyPostColorBar() { return "Post Color bar"; }
    mGlobal inline const char*	sKeyPostStart() { return "Post start"; }
    mGlobal inline const char*  sKeyPostStop() { return "Post stop"; }
    mGlobal inline const char*	sKeyPostTitleBox() { return "Post title box"; }
    mGlobal inline const char*	sKeyPostTraceNrs() { return "Post Trace Nrs"; }
    mGlobal inline const char*	sKeyRemarks() { return "Remarks"; }
    mGlobal inline const char*	sKeyResolution() { return "Resolution"; }
    mGlobal inline const char*	sKeyShape() { return "Shape"; }
    mGlobal inline const char*	sKeySkipWarning() { return "Skip Warning"; }
    mGlobal inline const char*	sKeyStartClipping() { return "Start Clipping"; }
    mGlobal inline const char*	sKeyUTMZone() { return "UTM zone"; }
    mGlobal inline const char*	sKeyWetFill() { return "Fill Wet"; }
    mGlobal inline const char*	sKeyWetFillColor() { return "Fill Color Wet"; }
    mGlobal inline const char*	sKeyWellNames() { return "Well names"; }
    mGlobal inline const char*	sKeyXRange() { return "X Range"; }
    mGlobal inline const char*	sKeyYRange() { return "Y Range"; }
    mGlobal inline const char*	sKeyZVals() { return "Z values"; }
    mGlobal inline const char*  sKeyFaultID() { return "FaultID"; }
    mGlobal inline const char*  sKeyHorizonID() { return "HorizonID"; }
    mGlobal inline const char*  sKeyZIntersectionYN() { return "ZIntersection"; }
    mGlobal inline const char*  sKeyUseFaultColorYN() { return "Use Fault Color"; }
    mGlobal inline const char*  sKeyFaultColor() { return "Fault Color"; }
    mGlobal inline const char*  sKeyUseWellSymbolsYN() { return "Use Well Symbols"; }
    mGlobal inline const char*  sKeyWellSymbolName() { return "Symbol Name"; }
};


mClass GMTWellSymbol : public NamedObject
{
public:
    BufferString	iconfilenm_;
    BufferString	deffilenm_;

    bool		usePar(const IOPar&);

    static const char*	sKeyIconFileName();
    static const char*	sKeyDefFileName();
};


mClass GMTWellSymbolRepository
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


mGlobal const GMTWellSymbolRepository& GMTWSR();


#define mGetDefault( key, fn, var ) \
    Settings::fetch("GMT").fn(key,var); \
    

#define mSetDefault( key, fn, var ) \
    Settings::fetch("GMT").fn(key,var); \
    Settings::fetch("GMT").write();

#endif

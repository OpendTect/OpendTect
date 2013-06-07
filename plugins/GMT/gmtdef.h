#ifndef gmtdef_h
#define gmtdef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtdef.h,v 1.17 2011/05/12 06:40:39 cvsnageswara Exp $
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

    static const char*	sShapeKeys[] = { "a", "c", "d", "s", "t", "x", "n",
					 "-", 0 };
    static const char*	sResolKeys[] = { "f", "h", "i", "l", "c" };

    static const char*	sKeyAttribName = "Attribute name";
    static const char*	sKeyClipOutside = "Clip outside";
    static const char*	sKeyClosePS = "Close PostScript";
    static const char*	sKeyColSeq = "Color sequence";
    static const char*	sKeyCustomComm = "Custom command";
    static const char*	sKeyDataRange = "Data range";
    static const char*	sKeyDrawContour = "Draw contour";
    static const char*	sKeyDrawGridLines = "Draw gridlines";
    static const char*	sKeyDryFill = "Fill Dry";
    static const char*	sKeyDryFillColor = "Fill Color Dry";
    static const char*	sKeyFill = "Fill";
    static const char*	sKeyFillColor = "Fill Color";
    static const char*	sKeyFlipColTab = "Flip color table";
    static const char*	sKeyFontSize = "Font size";
    static const char*	sKeyGMT = "GMT";
    static const char*	sKeyGMTSelKey = "808080";
    static const char*	sKeyGroupName = "Group Name";
    static const char*	sKeyLabelAlignment = "Label alignment";
    static const char*	sKeyLabelIntv = "Label Interval";
    static const char*	sKeyLegendParams = "Legend Parameters";
    static const char*	sKeyLineNames = "Line names";
    static const char*	sKeyLineStyle = "Line Style";
    static const char*	sKeyMapDim = "Map Dimension";
    static const char*	sKeyMapScale = "Map scale";
    static const char*	sKeyMapTitle = "Map Title";
    static const char*	sKeyPostLabel = "Post label";
    static const char*	sKeyPostColorBar = "Post Color bar";
    static const char*	sKeyPostStart = "Post start";
    static const char*  sKeyPostStop = "Post stop";
    static const char*	sKeyPostTitleBox = "Post title box";
    static const char*	sKeyPostTraceNrs = "Post Trace Nrs";
    static const char*	sKeyRemarks = "Remarks";
    static const char*	sKeyResolution = "Resolution";
    static const char*	sKeyShape = "Shape";
    static const char*	sKeySkipWarning = "Skip Warning";
    static const char*	sKeyStartClipping = "Start Clipping";
    static const char*	sKeyUTMZone = "UTM zone";
    static const char*	sKeyWetFill = "Fill Wet";
    static const char*	sKeyWetFillColor = "Fill Color Wet";
    static const char*	sKeyWellNames = "Well names";
    static const char*	sKeyXRange = "X Range";
    static const char*	sKeyYRange = "Y Range";
    static const char*	sKeyZVals = "Z values";
    static const char*  sKeyFaultID = "FaultID";
    static const char*  sKeyHorizonID = "HorizonID";
    static const char*  sKeyZIntersectionYN = "ZIntersection";
    static const char*  sKeyUseFaultColorYN = "Use Fault Color";
    static const char*  sKeyFaultColor = "Fault Color";
    static const char*  sKeyUseWellSymbolsYN = "Use Well Symbols";
    static const char*  sKeyWellSymbolName = "Symbol Name";
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

#ifndef gmtdef_h
#define gmtdef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: gmtdef.h,v 1.2 2008-08-06 09:58:20 cvsraman Exp $
________________________________________________________________________

-*/


namespace ODGMT
{
    enum Shape		{ Star, Circle, Diamond, Square, Triangle, Cross };
    			DeclareNameSpaceEnumUtils(Shape)

    static const char*	sShapeKeys[] = { "a", "c", "d", "s", "t", "x", 0 };

    static const char*	sKeyClosePS = "Close PostScript";
    static const char*	sKeyDataRange = "Data range";
    static const char*	sKeyDrawContour = "Draw contour";
    static const char*	sKeyFill = "Fill";
    static const char*	sKeyFillColor = "Fill Color";
    static const char*	sKeyGroupName = "Group Name";
    static const char*	sKeyLabelIntv = "Label Interval";
    static const char*	sKeyLegendParams = "Legend Parameters";
    static const char*	sKeyLineStyle = "Line Style";
    static const char*	sKeyMapDim = "Map Dimension";
    static const char*	sKeyMapTitle = "Map Title";
    static const char*	sKeyShape = "Shape";
    static const char*	sKeyXRange = "X Range";
    static const char*	sKeyYRange = "Y Range";
};


#endif

#ifndef SO_TRANSLATERECTANGLEDRAGGER_IV_H
#define SO_TRANSLATERECTANGLEDRAGGER_IV_H
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoTranslateRectangleDraggerGeom.h,v 1.4 2009/07/22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/

const char SoTranslateRectangleDragger::geombuffer[] =
  "#Inventor V2.1 ascii\n"
  "DEF translateTranslateRectangle\n"
  "Separator\n"
  "{\n"
      "ShapeHints\n"
      "{\n"
	    "vertexOrdering CLOCKWISE\n"
	    "shapeType UNKNOWN_SHAPE_TYPE\n"
      "}" 
      "Coordinate3\n"
      "{\n"
	  "point [ 0 -1 -1,\n"
		  "0 -1  1,\n"
		  "0  1  1,\n"
		  "0  1 -1,\n"
		  "0 -1 -1]\n"
      "}\n"
      "FaceSet\n"
      "{\n"
	  "numVertices 5\n"
      "}\n"
  "}\n";

#endif

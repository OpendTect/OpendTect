#ifndef SO_TRANSLATERECTANGLEDRAGGER_IV_H
#define SO_TRANSLATERECTANGLEDRAGGER_IV_H
/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoTranslateRectangleDraggerGeom.h,v 1.2 2003-11-07 10:04:25 bert Exp $
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

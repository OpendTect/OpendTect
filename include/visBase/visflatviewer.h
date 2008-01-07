#ifndef visflatviewer_h
#define visflatviewer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: visflatviewer.h,v 1.2 2008-01-07 20:49:37 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "visobject.h"

template <class T> class Array2D;
class SoSplitTexture2Part;



namespace visBase
{

class Coordinates;    
class MultiTexture2;
class FaceSet;
class VisColorTab;

/*!Implementation of FlatViewer::Viewer in 3D. */

class FlatViewer : public FlatView::Viewer, public VisualObjectImpl
{
public:
    static FlatViewer*	create()
			mCreateDataObj(FlatViewer);
    			~FlatViewer();

    Notifier<FlatViewer> dataChange;    

    void		handleChange(FlatView::Viewer::DataChangeType);
    void		setPosition(const Coord3& c00,const Coord3& c01,
				    const Coord3& c10,const Coord3& c11);
 

    mVisTrans* 		getDisplayTransformation();
    void		setDisplayTransformation(mVisTrans*);

    void		allowShading(bool yn);  
   				
protected:

    static int		nrBlocks(int totalnr,int maxnr,int overlap); 
    void		updateFaceSets(int rowsz,int colsz); 
    void		updateCoordinates(); 

    MultiTexture2*		texture_;
    Coordinates*		coords_;
    TypeSet<float>		c00factors_;
    TypeSet<float>		c01factors_;
    TypeSet<float>		c10factors_;
    TypeSet<float>		c11factors_;

    Coord3			c00_;
    Coord3			c01_;
    Coord3			c10_;
    Coord3			c11_;

    ObjectSet<FaceSet>		facesets_;
    ObjectSet<SoSplitTexture2Part> splittextures_;
};

}; // Namespace


#endif

#ifndef horizon3dtesselator_h
#define horizon3dtesselator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		August 2009
 RCS:		$Id: horizon3dtesselator.h,v 1.6 2012-08-03 13:00:27 cvskris Exp $
________________________________________________________________________


-*/

#include "geometrymod.h"
#include "position.h"
#include "task.h"

class Coord3List;


/* Assume given Coord3List has size nrrowscoord*nrcolscoord and all the points 
   are added in  order of rows. The nrrow and nrcol are the size you want to 
   tesselate. Your spacing should be 1, 2, 4, 8 ,16, or 32. The normals and 
   their indices are optional, you could put dummy 0s for them. The normals are
   calculated based on the spacing and they are local. The normal indices are 
   based on my own normals, hence normstartidx is provided in case you want to 
   arrange your global indices based on your spacing or whatever. */

mClass(Geometry) Horizon3DTesselator : public SequentialTask
{
public:
    			Horizon3DTesselator(const Coord3List*,
				int nrcoordcol,
				unsigned char spacing,
				int nrrow,int nrcol,
				TypeSet<int>* pointci,
				TypeSet<int>* lineci,
				TypeSet<int>* stripci,
				TypeSet<int>* pointni,
				TypeSet<int>* lineni,
				TypeSet<int>* stripni,
				Coord3List* normals,
				int nrnormalcols_,
				int normstartidx);
    int			nextStep();

protected:
    
    void		computeNormal(int ni,int row,int col);
    int			getNormalIdx(int crdidx);
    
    const Coord3List*	coords_;
    unsigned char	spacing_;
    int			nrcols_, nrrows_, nrcoordcols_;
    int			nrnormalcols_;

    TypeSet<int>*	pointci_;
    TypeSet<int>*	lineci_;
    TypeSet<int>*	stripci_;
    TypeSet<int>*	pointni_;
    TypeSet<int>*	lineni_;
    TypeSet<int>*	stripni_;
    Coord3List*		normals_;
    int			normalstart_;

    const double	cosanglexinl_, sinanglexinl_;
};



#endif


#ifndef faultstickset_h
#define faultstickset_h
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        J.C. Glas
Date:          November 2008
RCS:           $Id: faultstickset.h,v 1.5 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "rowcolsurface.h"

namespace Geometry
{

mClass FaultStickSet : public RowColSurface
{
public:
    			FaultStickSet();
    			~FaultStickSet();
    bool		isEmpty() const		{ return !sticks_.size(); }
    Element*		clone() const;

    virtual bool	insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int stick=0,
				    int firstcol=0);
    bool		removeStick(int stick);

    bool		insertKnot(const RCol&,const Coord3&);
    bool		removeKnot(const RCol&);

    int			nrSticks() const;
    int			nrKnots(int stick) const;

    StepInterval<int>	rowRange() const;
    StepInterval<int>	colRange(int stick) const;

    bool		setKnot(const RCol&,const Coord3&);
    Coord3		getKnot(const RCol&) const;
    bool		isKnotDefined(const RCol&) const;

    const Coord3&	getEditPlaneNormal(int stick) const;				
    enum ChangeTag	{StickChange=__mUndefIntVal+1,StickInsert,StickRemove};
    
    			// To be used by surface reader only
    void		addUdfRow(int stickidx,int firstknotnr,int nrknots);
    void		addEditPlaneNormal(const Coord3&);

    			// Use zscale=0 to measure in xy-direction only and
   			// zscale=MAXDOUBLE to measure in z-direction only.
    void		geometricStickOrder(TypeSet<int>& sticknrs,
				  double zscale,bool orderall=true) const;

    bool		isTwisted(int sticknr1,int sticknr2,
				  double zscale) const;

protected:
    double			interStickDist(int sticknr1,int sticknr2,
					       double zscale) const;

    int				firstrow_;

    ObjectSet<TypeSet<Coord3> >	sticks_;
    TypeSet<int>		firstcols_;
    
    TypeSet<Coord3>		editplanenormals_;
};

};

#endif

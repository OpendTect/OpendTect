#pragma once
                                                                                
/*+
________________________________________________________________________
(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        J.C. Glas
Date:          November 2008
________________________________________________________________________

-*/

/*
Info:
	sticknr: row nr corresponding to rowrange, can be negative
	stickidx: row index, starting from 0
*/

#include "geometrymod.h"
#include "refcount.h"
#include "rowcolsurface.h"

namespace Geometry
{

mExpClass(Geometry) FaultStickSet : public RowColSurface
{
public:
    			FaultStickSet();
    			~FaultStickSet();
    bool		isEmpty() const		{ return !sticks_.size(); }
    Element*		clone() const;

    virtual bool	insertStick(const Coord3& firstpos,
				    const Coord3& editnormal,int stick=0,
				    int firstcol=0);
    bool		removeStick(int sticknr);

    bool		insertKnot(const RowCol&,const Coord3&);
    bool		removeKnot(const RowCol&);

    int			nrSticks() const;
    int			nrKnots(int sticknr) const;
    const TypeSet<Coord3>* getStick(int stickidx) const;

    StepInterval<int>	rowRange() const;
    virtual StepInterval<int> colRange() const
    			{ return RowColSurface::colRange(); }
    StepInterval<int>	colRange(int stick) const;

    bool		setKnot(const RowCol&,const Coord3&);
    Coord3		getKnot(const RowCol&) const;
    bool		isKnotDefined(const RowCol&) const;

    const Coord3&	getEditPlaneNormal(int sticknr) const;
    enum ChangeTag	{ StickChange=__mUndefIntVal+1, StickInsert,
			  StickRemove, StickHide };
    
    			// To be used by surface reader only
    void		addUdfRow(int stickidx,int firstknotnr,int nrknots);
    void		addEditPlaneNormal(const Coord3&);

    			// Use zscale=0 to measure in xy-direction only and
   			// zscale=MAXDOUBLE to measure in z-direction only.
    void		geometricStickOrder(TypeSet<int>& sticknrs,
				  double zscale,bool orderall=true) const;

    bool		isTwisted(int sticknr1,int sticknr2,
				  double zscale) const;

    enum StickStatus	{ NoStatus=0, Selected=1, Preferred=2,
			  HiddenLowestBit=4 };

    void		selectStick(int sticknr,bool yn);
    bool		isStickSelected(int sticknr) const;
    void		preferStick(int sticknr);
    int			preferredStickNr() const;
    void		hideStick(int sticknr,bool yn,int sceneidx=-1);
    bool		isStickHidden(int sticknr,int sceneidx=-1) const;
    void		hideKnot(const RowCol&,bool yn,int sceneidx=-1);
    bool		isKnotHidden(const RowCol&,int sceneidx=-1) const;

protected:
    double			interStickDist(int sticknr1,int sticknr2,
					       double zscale) const;

    int				firstrow_;

    ObjectSet<TypeSet<Coord3> >	sticks_;
    TypeSet<int>		firstcols_;
    
    TypeSet<Coord3>		editplanenormals_;
    TypeSet<unsigned int>	stickstatus_;

    ObjectSet<TypeSet<unsigned int> > knotstatus_;
};

};


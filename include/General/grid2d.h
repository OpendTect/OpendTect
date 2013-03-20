#ifndef grid2d_h
#define grid2d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Jun 2010
 RCS:           $Id$
________________________________________________________________________

-*/


#include "generalmod.h"
#include "namedobj.h"
#include "position.h"

class HorSampling;
class IOPar;
class Line2;

/*!
\brief Represents a grid of 2D lines in an Inl-Crl plane.
*/

mExpClass(General) Grid2D
{
public:

    /*!\brief Line in an Inl-Crl plane.*/

    mExpClass(General) Line
    {
    public:
			Line(const BinID&,const BinID&);

	void		limitTo(const HorSampling&);
	bool		isReasonable() const;

	BinID		start_;
	BinID		stop_;
    };

    			Grid2D() {}
    virtual		~Grid2D();

    bool		isEmpty() const;
    int			size(bool dim) const;
    int			totalSize() const;
    const Grid2D::Line*	getLine(int idx,bool dim) const;

    void		set(const TypeSet<int>& inls,const TypeSet<int>& crls,
	    		    const HorSampling&);
    void		set(const Grid2D::Line&,double pardist,double perpdist,
	    		    const HorSampling&);

    void		limitTo(const HorSampling&);

protected:

    ObjectSet<Grid2D::Line>	dim0lines_;
    ObjectSet<Grid2D::Line>	dim1lines_;

    void		empty();
    void		createParallelLines(const Line2& baseline,double dist,
	    				    const HorSampling&,
					    ObjectSet<Grid2D::Line>& );
};


#endif


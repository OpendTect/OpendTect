#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"
#include "position.h"

class TrcKeySampling;
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

	void		limitTo(const TrcKeySampling&);
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
			    const TrcKeySampling&);
    void		set(const Grid2D::Line&,double pardist,double perpdist,
			    const TrcKeySampling&);

    void		limitTo(const TrcKeySampling&);

protected:

    ObjectSet<Grid2D::Line>	dim0lines_;
    ObjectSet<Grid2D::Line>	dim1lines_;

    void		empty();
    void		createParallelLines(const Line2& baseline,double dist,
					    const TrcKeySampling&,
					    ObjectSet<Grid2D::Line>& );
};

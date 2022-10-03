#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigeom.h"
#include "color.h"
#include "polygon.h"


mStruct(uiIo) SelectionArea
{
   enum SelAxisType		{ Y1, Y2, Both };

				SelectionArea(const uiRect&);
				SelectionArea(const ODPolygon<int>&);
				SelectionArea(bool isrect);
				SelectionArea();
				~SelectionArea();

    bool			isrectangle_	= true;
    bool			isInside(const uiPoint&) const;
    bool			isValid() const;
    Interval<double>		getValueRange(bool forx,bool alt=false) const;
    BufferStringSet		getAxisNames() const;
    uiPoint			center() const;
    float			selectedness(const uiPoint&) const;


    BufferString		xaxisnm_;
    BufferString		yaxisnm_;
    BufferString		altyaxisnm_;
    int				id_		= -1;
    uiRect			rect_;
    ODPolygon<int>		poly_;
    SelAxisType			axistype_	= Y1;
    uiWorldRect			worldrect_;
    ODPolygon<double>		worldpoly_;
    uiWorldRect			altworldrect_;
    ODPolygon<double>		altworldpoly_;
    uiPoint			center_;
    bool			operator==(const SelectionArea&) const;
    void			geomChanged()	{ maxdistest_ = mUdf(double); }
				// has to be called after pts changed only if
				// you want maxdistest_

protected:

    mutable double		maxdistest_	= mUdf(double);
    double			minDisToBorder(uiPoint) const;
				// only for inside pts
    double			maxDisToBorder() const;
};


mExpClass(uiIo) SelectionGrp : public NamedObject
{
public:
			SelectionGrp(const char* nm,const OD::Color& col);
			SelectionGrp(const char* nm=nullptr);
			~SelectionGrp();

    OD::Color		col_;
    int			size() const;
    bool		hasAltAxis() const;
    bool		isValidIdx(int idx) const;
    int			isInside(const uiPoint&) const;
			// return selarea id || - 1 if not selected
    int			validIdx(int selareaid) const;
    void		addSelection(const SelectionArea&);
    void		removeSelection(int);
    void		removeAll();

    void			setSelectionArea(const SelectionArea&);
    bool			getSelectionArea(SelectionArea&,int id) const;
    SelectionArea&		getSelectionArea(int idx);
    const SelectionArea&	getSelectionArea(int idx) const;
    SelectionArea::SelAxisType	getSelectionAxis(int selareaid) const;
    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;
    void			getInfo(BufferString&) const;

protected:
    TypeSet<SelectionArea> selareas_;
};

#ifndef uidpscrossplottools_h
#define uidpscrossplottools_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          June 2011
 RCS:           $Id: uidpscrossplottools.h,v 1.2 2011/07/11 11:50:16 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uigeom.h"
#include "polygon.h"

/*!\brief Data Point Set Cross Plotter */


mStruct SelectionArea
{
    enum SelAxisType	{ Y1, Y2, Both };

			    SelectionArea(const uiRect&);
			    SelectionArea(const ODPolygon<int>&);
			    SelectionArea(bool isrect);
			    SelectionArea() : axistype_(SelectionArea::Y1) {}
			    ~SelectionArea();

    bool			isrectangle_;
    bool			isInside(const uiPoint&) const;
    bool			isValid() const;
    Interval<double>		getValueRange(bool forx,bool alt=false) const;
    BufferStringSet		getAxisNames() const;
    uiPoint			center() const;
    float			selectedness(uiPoint) const;


    BufferString		xaxisnm_;
    BufferString		yaxisnm_;
    BufferString		altyaxisnm_;
    int 			id_;
    uiRect			rect_;
    ODPolygon<int>		poly_;
    SelAxisType			axistype_;
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

    mutable double		maxdistest_;
    double			minDisToBorder(uiPoint) const;
    				// only for inside pts 
    double			maxDisToBorder() const;
};


mClass SelectionGrp : public NamedObject
{
public:
				SelectionGrp(const char* nm, const Color& col)
				    : NamedObject(nm), col_(col)	{}
				SelectionGrp()
				    : NamedObject()			{}
				~SelectionGrp()				{}

	Color			col_;
	int			size() const;
	bool			hasAltAxis() const;
	bool			isValidIdx(int idx) const;
	int			isInside(const uiPoint&) const;
				// return selarea id || - 1 if not selected
	int			validIdx(int selareaid) const;
	void			addSelection(const SelectionArea&);
	void			removeSelection(int);
	void			removeAll();

	void			setSelectionArea(const SelectionArea&);
	bool			getSelectionArea(SelectionArea&,int id) const;
	SelectionArea&		getSelectionArea(int idx);
	const SelectionArea&	getSelectionArea(int idx) const;
	SelectionArea::SelAxisType getSelectionAxis(int selareaid) const;
	void			usePar(const IOPar&);
	void			fillPar(IOPar&) const;
	void			getInfo(BufferString&) const;
protected:
	TypeSet<SelectionArea> selareas_;
};

#endif

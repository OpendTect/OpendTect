#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "fontdata.h"
#include "namedobj.h"
#include "color.h"
#include "ranges.h"
#include "survinfo.h"

#include "bufstringset.h"


namespace Well
{

inline const char* sKey2DDispProp()	{ return "2D Display"; }
inline const char* sKey3DDispProp()	{ return "3D Display"; }

/*!
\brief Display properties of a well.
*/

mExpClass(Well) DisplayProperties
{
public:

			DisplayProperties(const char* subj = sKey3DDispProp());
			DisplayProperties(const Well::DisplayProperties& dp)
			{ *this = dp;}

    virtual		~DisplayProperties();

    DisplayProperties&	operator = (const DisplayProperties& dp);

    mStruct(Well) BasicProps
    {
			BasicProps( int sz=1 )
			    : size_(sz)
			    , color_(Color(0,0,255))	{}

	Color		color_;
	int		size_;

	void		usePar(const IOPar&);
	void		fillPar(IOPar&) const;
	void		useLeftPar(const IOPar&);
	void		useRightPar(const IOPar&);
	void		useCenterPar(const IOPar&);
	void		fillLeftPar(IOPar&) const;
	void		fillRightPar(IOPar&) const;
	void		fillCenterPar(IOPar&) const;

	virtual const char* subjectName() const		= 0;

    protected:

	virtual void	doUsePar(const IOPar&)		{}
	virtual void	doFillPar(IOPar&) const		{}
	virtual void	doUseLeftPar(const IOPar&)	{}
	virtual void	doFillRightPar(IOPar&) const	{}
	virtual void	doUseRightPar(const IOPar&)	{}
	virtual void	doFillLeftPar(IOPar&) const	{}
	virtual void	doUseCenterPar(const IOPar&)	{}
	virtual void	doFillCenterPar(IOPar&) const	{}

    };

    mStruct(Well) Track : public BasicProps
    {
			Track()
			    : BasicProps(1)
			    , dispabove_(true)
			    , dispbelow_(true)
			    , font_(10)
			    , nmsizedynamic_(true)
			    {}

	virtual const char* subjectName() const	{ return "Track"; }

	bool		dispabove_;
	bool		dispbelow_;
	bool		nmsizedynamic_;
	FontData	font_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;

    };

    mStruct(Well) Markers : public BasicProps
    {

			Markers()
			    : BasicProps(2)
			    , shapeint_(0)
			    , cylinderheight_(1)
			    , issinglecol_(false)
			    , font_(10)
			    , samenmcol_(true)
			    , nmsizedynamic_(true)
			    {}

	virtual const char* subjectName() const	{ return "Markers"; }

	int		shapeint_;
	int		cylinderheight_;
	bool		issinglecol_;
	FontData	font_;
	Color		nmcol_;
	bool		samenmcol_;
	BufferStringSet	selmarkernms_;
	bool		nmsizedynamic_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    mStruct(Well) Log : public BasicProps
    {
			Log()
			    : cliprate_(0)
			    , fillname_("none")
			    , fillrange_(mUdf(float),mUdf(float))
			    , isleftfill_(false)
			    , isrightfill_(false)
			    , isdatarange_(true)
			    , islogarithmic_(false)
			    , islogreverted_(false)
			    , issinglecol_(false)
			    , name_("none")
			    , logwidth_(250 *
				((int)(SI().xyInFeet() ? mToFeetFactorF:1)))
			    , range_(mUdf(float),mUdf(float))
			    , repeat_(5)
			    , repeatovlap_(50)
			    , seiscolor_(Color::White())
			    , seqname_("Rainbow")
			    , iscoltabflipped_(false)
			    , style_( 0 )
			    {}

	virtual const char* subjectName() const	{ return "Log"; }

	BufferString	name_;
	BufferString	fillname_;
	float           cliprate_;
	Interval<float> range_;
	Interval<float> fillrange_;
	bool		isleftfill_;
	bool		isrightfill_;
	bool            islogarithmic_;
	bool		islogreverted_;
	bool            issinglecol_;
	bool            isdatarange_;
	bool		iscoltabflipped_;
	int             repeat_;
	float           repeatovlap_;
	Color           linecolor_;
	Color		seiscolor_;
	BufferString    seqname_;
	int		logwidth_;
	int		style_;

    protected:

	virtual void	doUseLeftPar(const IOPar&);
	virtual void	doFillRightPar(IOPar&) const;
	virtual void	doUseRightPar(const IOPar&);
	virtual void	doFillLeftPar(IOPar&) const;
	virtual void	doUseCenterPar(const IOPar&);
	virtual void	doFillCenterPar(IOPar&) const;
    };

    Track		track_;
    Markers		markers_;
    bool		displaystrat_; //2d only

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    static DisplayProperties&	defaults();
    static void		commitDefaults();

    mStruct(Well) LogCouple { Log left_, right_, center_; };
    ObjectSet<LogCouple> logs_;

    virtual const char* subjectName() const	{ return subjectname_.buf(); }

protected:

    BufferString	subjectname_;

};

} // namespace


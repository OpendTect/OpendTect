#ifndef welldisp_h
#define welldisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id: welldisp.h,v 1.25 2010-04-23 08:40:31 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "color.h"
#include "ranges.h"

class IOPar;

namespace Well
{
    class D2TModel;
    class MarkerSet;

/*!\brief Display properties of a well */

mClass DisplayProperties
{
public:

			DisplayProperties()		{}

    mStruct BasicProps
    {
			BasicProps( int sz=1 )
			    : size_(sz)			
			    , color_(Color(0,0,255))
			    {}

	Color		color_;
	int		size_;

	void		usePar(const IOPar&);
	void		fillPar(IOPar&) const;
	void		useLeftPar(const IOPar&);
	void		useRightPar(const IOPar&);
	void		fillLeftPar(IOPar&) const;
	void		fillRightPar(IOPar&) const;
	virtual const char* subjectName() const		= 0;

    protected:

	virtual void	doUsePar(const IOPar&)		{}
	virtual void	doFillPar(IOPar&) const		{}
	virtual void	doUseLeftPar(const IOPar&) {}
	virtual void	doFillRightPar(IOPar&) const {}
	virtual void	doUseRightPar(const IOPar&){}
	virtual void	doFillLeftPar(IOPar&) const{}

    };

    mStruct Track : public BasicProps
    {
			Track()
			    : BasicProps(1)
			    , dispabove_(true)
			    , dispbelow_(true)	
			    , nmsize_(10)
		       	    {}


	virtual const char* subjectName() const		{ return "Track"; }

	bool		dispabove_;
	bool		dispbelow_;
	int 		nmsize_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    mStruct Markers : public BasicProps
    {

			Markers()
			    : BasicProps(5)
			    , shapeint_(0)	
			    , cylinderheight_(1)			
			    , issinglecol_(false)
			    , nmsize_(10)
			    , samenmcol_(false)		 
			    {}

	virtual const char* subjectName() const	{ return "Markers"; }
	int		shapeint_;
	int		cylinderheight_;
	bool 		issinglecol_;
	int 		nmsize_;
	Color		nmcol_;
	bool		samenmcol_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    mStruct Log : public BasicProps
    {
			Log()
			    : cliprate_(0)
			    , fillname_("none")
			    , fillrange_(mUdf(float),mUdf(float))
		            , isdatarange_(true)
			    , islogfill_(true)
			    , islogarithmic_(false) 
			    , issinglecol_(false)
			    , iswelllog_(true)	
			    , name_("none")
		            , logwidth_ (40)	
			    , range_(mUdf(float),mUdf(float))
			    , repeat_(5)
			    , repeatovlap_(50)
		            , seiscolor_(Color::White())
			    , seqname_("Rainbow")
			    {}		 

	virtual const char* subjectName() const	{ return "Log"; }

	BufferString	    name_;
	BufferString	    fillname_;
	bool		    iswelllog_;
	float               cliprate_;      
	Interval<float>     range_;        
	Interval<float>     fillrange_;       
	bool                logarithmic_;
	bool                islogfill_;
	bool                islogarithmic_;
	bool                issinglecol_;
	bool                isdatarange_;
	int                 repeat_;
	float               repeatovlap_;
	Color               linecolor_;
	Color               seiscolor_;
	BufferString        seqname_;
	int 		    logwidth_;


    protected:

	virtual void	doUseLeftPar(const IOPar&);
	virtual void	doFillRightPar(IOPar&) const;
	virtual void	doUseRightPar(const IOPar&);
	virtual void	doFillLeftPar(IOPar&) const;
    };

    void copyFrom( const DisplayProperties& d)
    {
	track_ = d.track_;
	markers_ = d.markers_;
	left_ = d.left_;
	right_ = d.right_;
    }

    Track		track_;
    Markers		markers_;
    Log			left_;
    Log			right_;
    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;


    static DisplayProperties&	defaults();
    static void			commitDefaults();

};


/*!\brief used for a 2D display of well data ( log, strat ... ) */
mClass Well2DDispData
{
public:
			    Well2DDispData()
			    : zrg_(mUdf(float),0)
			    , zistime_(false)
			    , dispzinft_(false)
			    , markers_(0)
			    , d2tm_(0)
			    {}

    void		    copyFrom(Well2DDispData& d)
			    {
				zrg_ 	 = d.zrg_;
				zistime_ = d.zistime_;
				dispzinft_ = d.dispzinft_;
				markers_ = d.markers_;
				d2tm_ 	 = d.d2tm_;
			    }	
    const Well::D2TModel*   d2tm_;
    Well::MarkerSet*        markers_;
    Interval<float>         zrg_;
    bool                    dispzinft_;
    bool                    zistime_;
};

} // namespace

#endif

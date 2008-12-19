#ifndef welldisp_h
#define welldisp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id: welldisp.h,v 1.6 2008-12-19 16:08:58 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "color.h"
#include "ranges.h"

class IOPar;

namespace Well
{

/*!\brief Display properties of a well */

class DisplayProperties
{
public:

			DisplayProperties()		{}


    struct BasicProps
    {
			BasicProps( int sz=1 )
			    : size_(sz)			{}

	Color		color_;
	int		size_;

	void		usePar(const IOPar&);
	void		fillPar(IOPar&) const;
	virtual const char* subjectName() const		= 0;

    protected:

	virtual void	doUsePar(const IOPar&)		{}
	virtual void	doFillPar(IOPar&) const		{}

    };

    struct Track : public BasicProps
    {
			Track()
			    : dispabove_(true)
			    , dispbelow_(false)		{}

	virtual const char* subjectName() const		{ return "Track"; }

	bool		dispabove_;
	bool		dispbelow_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    struct Markers : public BasicProps
    {

			Markers()
			    : BasicProps(8)
			    , circular_(true)	{}

	virtual const char* subjectName() const	{ return "Markers"; }
	bool		circular_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    struct Log : public BasicProps
    {

			Log()
			    : name_("none")
			    , fillname_("none")
			    , iswelllog_(true)	
			    , cliprate_(mUdf(float))
			    , range_(mUdf(float),mUdf(float))
			    , fillrange_(mUdf(float),mUdf(float))
		            , isdatarange_(true)
			    , logarithmic_(false)
			    , repeat_(1)
			    , repeatovlap_(mUdf(float))
			    , linecolor_(Color::White)
			    , islogfill_(false)
		            , seiscolor_(Color::White)
			    , seqname_("AI")
			    , issinglecol_(false)		    
							{}

	BufferString	    name_;
	BufferString	    fillname_;
	bool		    iswelllog_;

	float               cliprate_;      //!< If undef, use range_
	Interval<float>     range_;         //!< If cliprate_ set, filled using it
	Interval<float>     fillrange_;       
	bool                logarithmic_;
	bool                isdatarange_;
	bool                islogfill_;
	bool                issinglecol_;
	int                 repeat_;
	float               repeatovlap_;
	Color               linecolor_;
	Color               seiscolor_;
	const char*         seqname_;

    

	virtual const char* subjectName() const	{ return "Log"; }

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    Track		track_;
    Markers		markers_;
    Log			left_;
    Log			right_;
    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;


    static DisplayProperties&	defaults();
    static void			commitDefaults();

};


} // namespace

#endif

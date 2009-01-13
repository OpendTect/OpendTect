#ifndef welldisp_h
#define welldisp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id: welldisp.h,v 1.11 2009-01-13 08:23:43 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "color.h"
#include "ranges.h"

class IOPar;

namespace Well
{

/*!\brief Display properties of a well */

mClass DisplayProperties
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

    struct Track : public BasicProps
    {
			Track()
			    : dispabove_(true)
			    , dispbelow_(true)		{}

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
			    , circular_(true)	
			    , issinglecol_(false)
						{}

	virtual const char* subjectName() const	{ return "Markers"; }
	bool		circular_;
	bool 		issinglecol_;

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
			    , logarithmic_(false)
			    , repeat_(5)
			    , repeatovlap_(50)
			    , linecolor_(Color::White())
			    , islogfill_(true)
		            , isdatarange_(true)
		            , seiscolor_(Color::White())
			    , seqname_("Rainbow")
			    , issinglecol_(false)	{}

	virtual const char* subjectName() const	{ return "Log"; }

	BufferString	    name_;
	BufferString	    fillname_;
	bool		    iswelllog_;
	float               cliprate_;      
	Interval<float>     range_;        
	Interval<float>     fillrange_;       
	bool                logarithmic_;
	bool                islogfill_;
	bool                issinglecol_;
	bool                isdatarange_;
	int                 repeat_;
	float               repeatovlap_;
	Color               linecolor_;
	Color               seiscolor_;
	const char*         seqname_;


    protected:

	virtual void	doUseLeftPar(const IOPar&);
	virtual void	doFillRightPar(IOPar&) const;
	virtual void	doUseRightPar(const IOPar&);
	virtual void	doFillLeftPar(IOPar&) const;
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

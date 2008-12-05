#ifndef welldisp_h
#define welldisp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id: welldisp.h,v 1.2 2008-12-05 14:43:58 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "color.h"

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
			    : BasicProps(5)
			    , circular_(true)	{}

	virtual const char* subjectName() const	{ return "Markers"; }
	bool		circular_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    Track		track_;
    Markers		markers_;

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;


    static DisplayProperties&	defaults();
    static void			commitDefaults();

};


} // namespace

#endif

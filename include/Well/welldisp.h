#ifndef welldisp_h
#define welldisp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id: welldisp.h,v 1.1 2008-12-05 12:58:58 cvsbert Exp $
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

	virtual void	usePar(const IOPar&);
	virtual void	fillPar(IOPar&) const;
	virtual const char* sIOParKey() const		= 0;

	static int	defaultSize();
    };

    struct Track : public BasicProps
    {
			Track()
			    : dispabove_(true)
			    , dispbelow_(false)		{}

	virtual const char* sIOParKey() const		{ return "Track"; }
	virtual void	usePar(const IOPar&);
	virtual void	fillPar(IOPar&) const;

	bool		dispabove_;
	bool		dispbelow_;
    };

    struct Markers : public BasicProps
    {
			Markers() : BasicProps(5)	{}

	virtual const char*	sIOParKey() const	{ return "Markers"; }
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

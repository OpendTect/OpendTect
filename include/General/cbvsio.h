#ifndef cbvsio_h
#define cbvsio_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format io
 RCS:		$Id: cbvsio.h,v 1.2 2001-04-04 15:06:52 bert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>


/*!\brief Base class for CBVS reader and writer */

class CBVSIO
{
public:

			CBVSIO()
			: errmsg_(0)
			, nrcomps(0), cnrbytes_(0)	{}
			~CBVSIO()			{ delete [] cnrbytes_; }

    bool		failed() const			{ return errmsg_; }
    const char*		errMsg() const			{ return errmsg_; }

    virtual void	close() 			= 0;

protected:

    const char*		errmsg_;
    int			nrcomps;
    int*		cnrbytes_;

    static const int	integersize;
    static const int	version;
    static const int	headstartbytes;

};


#endif

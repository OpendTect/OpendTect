#ifndef cbvsio_h
#define cbvsio_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format io
 RCS:		$Id: cbvsio.h,v 1.3 2001-04-05 16:21:35 bert Exp $
________________________________________________________________________

-*/

#include <position.h>


/*!\brief Base class for CBVS reader and writer */

class CBVSIO
{
public:

			CBVSIO()
			: errmsg_(0), strmclosed_(false), nrxlines_(1)
			, nrcomps_(0), cnrbytes_(0)	{}
			~CBVSIO()			{ delete [] cnrbytes_; }

    bool		failed() const			{ return errmsg_; }
    const char*		errMsg() const			{ return errmsg_; }

    virtual void	close() 			= 0;
    int			nrComponents() const		{ return nrcomps_; }
    const BinID&	binID() const			{ return curbinid_; }

protected:

    const char*		errmsg_;
    int*		cnrbytes_;
    BinID		curbinid_;
    int			nrcomps_;
    bool		strmclosed_;
    int			nrxlines_;

    static const int	integersize;
    static const int	version;
    static const int	headstartbytes;

};


#endif

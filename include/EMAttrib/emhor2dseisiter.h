#ifndef emhor2dseisiter_h
#define emhor2dseisiter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2009
 RCS:           $Id: emhor2dseisiter.h,v 1.1 2009-10-09 08:39:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "multiid.h"
class Seis2DLineSet;
namespace EM { class Horizon2D; class Horizon2DGeometry; }


namespace EM
{

class Hor2DSeisLineIterator
{
public:

			Hor2DSeisLineIterator(const MultiID&);
    virtual		~Hor2DSeisLineIterator();

    bool		next();
    bool		isValid() const;
    void		reset();
    void		getLineSet();
    int			nrLines() const			{ return nrsticks_; }
    int			nrLinesDone() const		{ return stickidx_+1; }
    int			lineID() const;
    const char*		lineName() const;

    Horizon2D*		horizon()			{ return h2d_; }
    const Horizon2D*	horizon() const			{ return h2d_; }
    Horizon2DGeometry&	geometry()			{ return *geom_; }
    const Horizon2DGeometry& geometry() const		{ return *geom_; }
    Seis2DLineSet*	lineSet()			{ return lset_; }
    const Seis2DLineSet* lineSet() const		{ return lset_; }

protected:

    Horizon2D*		h2d_;
    Horizon2DGeometry*	geom_;
    int			stickidx_;
    const int		nrsticks_;
    Seis2DLineSet*	lset_;
    MultiID		curlsid_;

};

} // namespace EM

#endif

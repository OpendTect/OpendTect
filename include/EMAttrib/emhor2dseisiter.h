#ifndef emhor2dseisiter_h
#define emhor2dseisiter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2009
 RCS:           $Id: emhor2dseisiter.h,v 1.5 2010/11/15 09:35:45 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "multiid.h"
class Seis2DLineSet;
namespace EM { class Horizon2D; class Horizon2DGeometry; }


namespace EM
{

mClass Hor2DSeisLineIterator
{
public:

			Hor2DSeisLineIterator(const Horizon2D&);
			Hor2DSeisLineIterator(const MultiID&);
    virtual		~Hor2DSeisLineIterator();

    bool		next();
    bool		isValid() const;
    void		reset();
    void		getLineSet();
    int			nrLines() const			{ return nrlines_; }
    int			nrLinesDone() const		{ return lineidx_+1; }
    const char*		lineName() const;
    int			lineSetIndex(const char* attrnm=0) const;
    const MultiID&	lineSetKey()			{ return curlsid_; }

    const Horizon2D*	horizon() const			{ return h2d_; }
    const Horizon2DGeometry& geometry() const		{ return *geom_; }
    Seis2DLineSet*	lineSet()			{ return lset_; }
    const Seis2DLineSet* lineSet() const		{ return lset_; }

protected:

    const Horizon2D*		h2d_;
    const Horizon2DGeometry*	geom_;
    int				lineidx_;
    const int			nrlines_;
    Seis2DLineSet*		lset_;
    MultiID			curlsid_;

private:

    void		init(const Horizon2D*);

};

} // namespace EM

#endif

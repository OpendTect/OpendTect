#ifndef viscolortabindexer_h
#define viscolortabindexer_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		January 2007
 RCS:		$Id: viscolortabindexer.h,v 1.1 2007-01-03 18:22:17 cvskris Exp $
________________________________________________________________________


-*/

#include "basictask.h"

namespace Threads { class Mutex; }

namespace visBase
{

class VisColorTab;

/*!\brief
Bins float data according a colortable's table-colors. Number of bins is
dependent on number of entries in the colortable's table. Undef-values are
assigned nrStep() as index, and are not present in the histogram.

*/


class TextureColorTabIndexer : public ParallelTask
{
public:
	    			TextureColorTabIndexer( const float* inp,
				    unsigned char* outp, int sz,
				    const VisColorTab* );

				~TextureColorTabIndexer();

    const unsigned int*		getHistogram() const;
    int				nrHistogramSteps() const;

protected:
    bool			doWork(int start,int stop,int threadid);
    int				nrTimes() const;

    unsigned char*		indexcache_;
    const float*		datacache_;
    const visBase::VisColorTab*	colortab_;
    unsigned int*		globalhistogram_;
    int				nrhistogramsteps_;
    Threads::Mutex&		histogrammutex_;
    const int			sz_;
};


}; // Namespace


#endif

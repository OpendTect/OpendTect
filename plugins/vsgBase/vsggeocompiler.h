#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/core/Inherit.h>
#include <vsg/core/ref_ptr.h>

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

namespace vsg { class Viewer; }

namespace vsgGeo
{

/*!\brief Compiles subgraphs that are added to a running scene, and keeps
  removed subgraphs alive until the GPU no longer uses them.*/

class VSGGEO_EXPORT Compiler : public vsg::Inherit<vsg::Object,Compiler>
{
public:
			Compiler();

    void		add(vsg::ref_ptr<vsg::Object>);
    void		release(vsg::ref_ptr<vsg::Object>);

    void		setReleaseDelay(uint64_t nrframes);
    uint64_t		releaseDelay() const	{ return releasedelay_; }

    size_t		nrPendingCompiles() const;
    size_t		nrPendingReleases() const;
    int			nrFailedCompiles() const { return nrfailed_; }

    void		process(vsg::Viewer&);
    void		releaseAll();

protected:
			~Compiler();

private:

    bool		compileObject(vsg::Viewer&,vsg::ref_ptr<vsg::Object>);

    struct Retired
    {
	uint64_t			frame_;
	vsg::ref_ptr<vsg::Object>	obj_;
    };

    mutable std::mutex			lock_;
    std::vector<vsg::ref_ptr<vsg::Object>> tocompile_;
    std::deque<Retired>			retired_;
    uint64_t				releasedelay_ = 4;
    uint64_t				lastframe_ = 0;
    int					nrfailed_ = 0;
};

} // namespace vsgGeo

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeocompiler.h"

#include <vsg/core/observer_ptr.h>
#include <vsg/io/Options.h>
#include <vsg/threading/OperationQueue.h>

namespace vsg { class Viewer; }

namespace vsgGeo
{

/*!\brief vsgGeo object with changes that have to be applied between frames,
  for example a subgraph that must be rebuilt and recompiled. */

class VSGGEO_EXPORT Updatable : public vsg::Inherit<vsg::Object,Updatable>
{
public:
    virtual void	applyUpdates(vsg::Viewer&,Compiler&)	= 0;
};


/*!\brief Runs once per frame, on the thread that renders, before the frame
  is recorded: applies pending updates, compiles new subgraphs and frees
  released ones.

  Use install() once per viewer.
*/

class VSGGEO_EXPORT Updater : public vsg::Inherit<vsg::Operation,Updater>
{
public:

    static vsg::ref_ptr<Updater> install(vsg::Viewer&);

    Compiler&		compiler()		{ return *compiler_; }
    const Compiler&	compiler() const	{ return *compiler_; }

    vsg::ref_ptr<vsg::Options> sharedOptions() const { return options_; }
			/*!<Of this viewer: what is built with them is shared,
			    and goes when the viewer goes. */

    void		requestUpdate(vsg::ref_ptr<Updatable>);
    size_t		nrPendingUpdates() const;

    void		run() override;

			Updater(vsg::Viewer&);
protected:
			~Updater();

private:

    vsg::observer_ptr<vsg::Viewer>		viewer_;
    vsg::ref_ptr<Compiler>			compiler_;
    vsg::ref_ptr<vsg::Options>			options_;
    mutable std::mutex				lock_;
    std::vector<vsg::ref_ptr<Updatable>>	updates_;
};

} // namespace vsgGeo

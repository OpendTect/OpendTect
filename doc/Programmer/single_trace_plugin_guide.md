# Single Trace Processing Plugin Guide

This walkthrough shows how to turn the stock plugin template into a deployable single-trace processor. It leans on the tutorial plugins (`plugins/Tut`, `plugins/uiTut`) and the reusable `SeisSingleTraceProc` executor that already encapsulates I/O, threading and progress handling.

---

## Step 1 – Bootstrap a plug-in workspace

1. **Copy the template project** from `doc/Programmer/pluginexample` to a new folder (for example `~/my_single_trace_plugin`). Keep its `CMakeLists.txt` as the starting point.
2. **Rename the modules** listed in `PLUGINS` and update the per-module metadata (version/product/package) so that CMake generates the correct targets and ALO files, following the existing loop in the template.
3. **Point CMake to your OpendTect installation** via `-D OpendTect_DIR=/path/to/od/bundle` when configuring out-of-tree builds. Leave `OD_BUILD_LOCAL` enabled while iterating so the plug-in binaries drop next to the build tree.

---

## Step 2 – Register modules and metadata

1. Inside each non-UI module, add a `<name>pi.cc` that implements the standard `mDefODPluginEarlyLoad`, `mDefODPluginInfo`, and `mDefODInitPlugin` macros (see `plugins/Tut/tutpi.cc` for the minimal pattern). Use `mDefODInitPlugin` to call every `initClass()` or other factory registration your plug-in needs.
2. Keep backend/UI modules separate (e.g., `MyTraceOps` versus `uiMyTraceOps`). The UI module should depend on the backend module so that runtime type registrations happen exactly once.

---

## Step 3 – Create the single-trace executor wrapper

1. **Instantiate `SeisSingleTraceProc`:** Construct it with one or two `SeisStoreAccess::Setup` objects that describe the input/output seismic stores (`IOObj`, `GeomID`, component selection, `Seis::SelData`, coordinate system). Reuse the helper logic from `uiSeisTransfer::getTrcProc()` when building those setups from a selection dialog.
2. **Attach to the lifecycle notifiers:**
   - `inputready_` fires after the readers and writer are initialised; use it to inspect `reader()`/`writer()` and cache geometry-specific helpers.
   - `traceselected_` fires after metadata is fetched but before the samples are read. This is the earliest point to call `skipCurTrc()` when you need to drop traces based on headers only.
   - `proctobedone_` fires right after `SeisSingleTraceProc` has copied the incoming trace into `worktrc_` and before it is written. Attach your processing callback here and edit `getTrace()` in place.
3. **Opt in to built-in helpers** by calling `setScaler()`, `setResampler()`, `skipNullTraces()`, `fillNullTraces()`, `setExtTrcToSI()`, or `setProcPars()` so that you do not need to re-implement per-trace book-keeping.
4. **Expose execution control** such as `setTracesPerStep()` or `setTotalNrIfUnknown()` if your UI needs to influence throughput or progress estimation.

---

## Step 4 – Implement the per-trace algorithm

1. Create a small helper class (similar to `SeisCubeCopier`) that owns the `SeisSingleTraceProc`, attaches the callbacks, and exposes an `Executor` interface.
2. In the `proctobedone_` callback, retrieve and mutate the trace:

   ```c++
   void MyTraceOp::process(CallBacker*)
   {
       SeisTrc& trc = stp_->getTrace();
       const SeisTrc& inptrc = stp_->getInputTrace(); // if you need unscaled samples
       // Example: multiply every sample by a user-defined factor
       SeisTrcPropChg changer( trc );
       changer.scale( params_.factor, params_.shift );
   }
   ```

3. If your operation requires external metadata (velocity models, well logs, etc.), load and cache them during the `inputready_` callback and free the resources when the executor is destroyed.
4. Use `skipCurTrc()` inside the `traceselected_` callback to drop traces based on headers (e.g., only process traces whose inline is in a QA list).
5. Always detach callbacks in your wrapper’s destructor (`detachAllNotifiers()`) before deleting the `SeisSingleTraceProc` instance.

---

## Step 5 – Surface parameters in the UI or command line

1. Copy the structure of `uiTutSeisTools`: build a `uiDialog` that gathers (a) input seismic object, (b) spatial/z selection, (c) algorithm parameters, and (d) output object.
2. Instantiate your executor with those parameters, then run it through `uiTaskRunner` so that progress, cancellation, and error messaging reuse OpendTect’s standard mechanisms.
3. For headless scripts, expose the same knobs via `IOPar` parsing and feed them into `SeisSingleTraceProc::setProcPars()` plus your own parameter struct to keep behaviour consistent between UI and batch runs.

---

## Step 6 – Build, deploy, and test

1. Configure & build the plug-in project:

   ```bash
   cmake -S /path/to/my_single_trace_plugin \
         -B /path/to/build \
         -D OpendTect_DIR=/opt/OpendTect-7.x \
         -D OD_BINARY_BASEDIR=/opt/OpendTect-7.x
   cmake --build /path/to/build --target install_plugins
   ```

2. Point `OD_USER_PLUGIN_DIR` to your build output (the template’s launchers already do this) and start `od_main` to verify that the plug-in appears under **Utilities → Plugins**.
3. Run a small 2D and 3D dataset through the new processor. Confirm that skipped traces, re-sampled traces, and null-trace handling behave as expected. Check the output object’s component names to ensure they are forwarded correctly.

---

### Debugging & hardening checklist

* Set `skipNullTraces()` and/or `fillNullTraces()` according to how you want to treat missing input traces.
* Use `setScaler()` only if you need post-processing scale/shift; otherwise keep the reader in its fastest I/O mode.
* If you change z-sampling, call `setResampler()` to resample the input trace prior to your processing logic instead of manual looping.
* For performance, increase `setTracesPerStep()` once you confirm your algorithm is thread-safe and fast enough.
* Attach unit-style regression tests by reusing the tutorial executor (`Tut::SeisTools`) as a reference implementation until your algorithm stabilises.

Following these steps gives you a reproducible path from an empty template to a fully integrated single-trace processing plug-in that fits within the existing OpendTect execution, UI, and distribution pipelines.

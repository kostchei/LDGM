# ADR 0004: Embedded debug information for isolated Windows builds

- Status: Accepted as a containment measure; not a complete crash fix
- Date: 2026-07-15
- Scope: Reproducible Windows builds

## Context

MSVC 19.44 repeatedly exited with `0xC0000005` while compiling large O3DE unity translation units. The later failure stack ended in `CL!CloseTypeServerPDB`, even with one MSBuild project, one compiler worker and per-source compiler invocations. Profile builds used `/Zi`, which routes compiler debug information through the program-database type-server path implicated by the crash.

## Decision

When `LDGMDisableCompilerBatching=true`, the checked-in `Directory.Build.targets` selects MSBuild's `OldStyle` debug-information format. This emits embedded CodeView debug information with `/Z7`. The override is limited to the existing isolated-build mode; normal build configurations retain their generated O3DE setting.

## Consequences

- Isolated builds avoid the compiler PDB type-server path while retaining debug symbols in object files for the linker.
- Object files may be larger than `/Zi` objects.
- The workaround can be removed after a pinned MSVC upgrade is verified against the full O3DE client/server build.

## Verification status

The generated AzCore compiler command contains `/Z7`, confirming that the MSBuild property is applied in isolated mode. The original `CL!CloseTypeServerPDB` signature did not recur, but MSVC still exited with `0xC0000005` in other AzCore unity translation units.

During an earlier incremental verification attempt, Windows bugchecked and restarted (bugcheck `0x1E`, exception `0xC0000005`; a WHEA event 19 corrected processor-core parity error preceded it). The CPU microcode has since been patched and the host is treated as stable.

On 2026-07-15 the full `/Z7` recompile of the framework, test and launcher targets completed. Two isolated `cl.exe` crashes occurred during the recompile (`0xC000001D` in AzCore, `0xC0000005` in AzFramework); neither recurred at the same translation unit and immediate retries completed cleanly. The 6 Gem unit tests pass and both launchers pass the bounded runtime probes. `/Z7` is therefore accepted as removing the type-server crash path, while transient compiler crashes on very large unity translation units remain possible and are contained by retrying. Candidate structural fixes, to be decided in a separate ADR: configuring `LY_UNITY_BUILD=OFF`, or pinning an MSVC 19.3x toolset.

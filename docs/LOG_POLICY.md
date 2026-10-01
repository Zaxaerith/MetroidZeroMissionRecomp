# Final preview logging policy

No whole-route, per-instruction or per-memory-access trace, trajectory sweep,
frame bisection or coverage research. For a real bug, consult decomp first and
record only a bounded hypothesis-specific window or subsystem.

Final cleanup retains the latest successful host build log and one compact cold
validation report plus the three current smoke result JSONs. Old logs, research archives, screenshots, frame/state dumps,
WAVs and binary backups are removed after their conclusions enter documentation.
Generated/build/runtime caches are reproducible local artifacts and removed
again after cold validation. They remain ignored when users build locally.

Progress records concrete acceptance and AOT/healed/interpreted/miss values;
there is no subjective percentage or zero-miss release prerequisite.

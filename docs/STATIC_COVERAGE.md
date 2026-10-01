# Coverage snapshot, not a release gate

Current generated scale:20035 functions (ARM94/Thumb19941), plus7 guarded native
moving-stack bodies. Native execution can use interpreter fallback; no complete
strict-static claim is made or required. Ordinary AOT executed-instruction
counters are unavailable; native_calls measures healed-cache execution.

COVERAGE_CURRENT.json identifies the exact canonical route/build/fixture and
AOT/healed/interpreted/miss state. Compare counters only on the same route.
Past strict first-miss observations are informational; no miss-by-miss sweep
is scheduled. Consult decomp first when a real bug needs correction.

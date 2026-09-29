# External Validation of the Transport Controllers

Internal tests prove consistency; this validation checks the controller equations
against independent public references.

## References

- Reno: RFC 5681 and `TCPReno` from TL-System `ns.py` v0.4.5.
- CUBIC: RFC 8312/9438 equations, Linux conventions (`C=0.4`,
  `beta_cubic=0.7`) and `TCPCubic` from the same independent simulator.
- RTO: RFC 6298 estimators, with the documented Linux 200 ms minimum instead of
  the RFC's conservative 1 s floor.
- Prague: the external L4STeam `udp_prague/prague_cc.cpp` is compiled unchanged
  behind the local C ABI shim; there is no Python reimplementation to drift.

`benchmarks/validate_tcp_references.py` drives the local and ns.py controllers
through identical ACK/loss sequences and evaluates independent RFC vectors.

## Reproducing

```bash
git clone --depth 1 --branch v0.4.5 \
  https://github.com/TL-System/ns.py.git /tmp/ns.py

cd /path/to/5g-network-emulator
make -C transport/prague
PYTHONPATH=transport python3 \
  transport/benchmarks/validate_tcp_references.py \
  --ns-root /tmp/ns.py \
  --output transport/benchmarks/results/tcp-reference.json
```

## Acceptance

- Reno cwnd/ssthresh match for 500 ACKs, fast retransmit/recovery and RTO.
- CUBIC matches for 1000 ACKs within one MSS; the measured run currently has
  zero byte error.
- CUBIC curve point at `W=100 MSS`, `t=1.05 s` matches
  `100 + 0.4 * 1.05^3`.
- Multiplicative decrease uses `beta_cubic=0.7`.
- RFC 6298 samples `(100 ms, 120 ms)` produce the expected SRTT, RTTVAR and RTO.
- Prague loads the reference implementation and reports its configured MSS/IW.

This validates congestion-control state transitions and equations. It does not
claim packet-for-packet equivalence with a complete Linux TCP stack: Linux also
has HyStart++, PRR, RACK/TLP, implementation-specific pacing and socket-buffer
behavior that this intentionally smaller offline model does not implement.

# CanorInfinitum: Project C (Hardware, Software & Technology)

Project C: All of CanorInstinctivus-Hardware, -Software, and -Technology. Exact unbounded arithmetic, big integers, factoradic/Lehmer, primes, surreals, Golay coding, and Zephyr / bare-metal integration.

## Role in Canor Architecture
- **Layer**: Defined in \Canor_Working-Draft_v1.pdf\
- **Language / Convention**: C11 with integer-only arithmetic, \canor_*\ naming.

## Key Modules & Legacy Mapping
| Legacy Source (\D:\Workspace\.old\follow_core-verifiable_intelligence\\\) | Target Module | Description |
|---|---|---|
| fc_big -> canor_big, fc_prime -> canor_prime, fc_qval -> canor_qval, fc_b2i -> canor_b2i, vi_golay -> canor_golay | Active migration | See [CONTEXT.md](CONTEXT.md) |

## Cross References
- [Root README](../../README.md)
- [Local AGENTS Rules](AGENTS.md)
- [Local Context & Mappings](CONTEXT.md)
- [Global Schema](../../SCHEMA.md)
- [Global Architecture](../../DESIGN.md)

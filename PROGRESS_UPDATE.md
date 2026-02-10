# Test Fix Progress Update

## Results After Round 1 Fixes

**Before**: 58/81 passing (71.6%), 23 failing  
**After Round 1**: 64/81 passing (79.0%), 17 failing  
**Improvement**: +6 tests fixed, +7.4% pass rate

## Tests Successfully Fixed (6 tests)
✅ testE2E_Inbound_PcieWrite_TlbApp1_NocN  
✅ testE2E_Outbound_NocN_TlbAppOut0_Pcie  
✅ testE2E_Outbound_SmnN_TlbSysOut0_Pcie  
✅ testE2E_Outbound_NocN_TlbAppOut1_PcieDBI  
✅ testE2E_Flow_NocMemoryRead_ToPcie  
✅ testE2E_Flow_SmnConfigWrite_PcieDBI  

## Remaining 17 Failures

### Category 1: Transaction Failures (8 tests) - ADDRESS RANGE ISSUE
These tests are failing with "transaction should complete" (ok==false):
1. testE2E_Inbound_PcieRead_TlbApp0_NocN
2. testE2E_Concurrent_InboundOutbound  
3. testE2E_Concurrent_MultipleTlbs
4. testE2E_Flow_PcieMemoryRead_Complete
5. testE2E_Flow_PcieMemoryWrite_Complete
6. testE2E_Perf_MaximumThroughput (count mismatch due to failed transactions)
7. testE2E_Stress_TlbEntryExhaustion

**Root Cause**: Addresses used might be intercepted by SCML2 mirror model or are invalid.  
**Solution**: Need to use addresses in working range (like 0x18900000 as used in passing tests).

### Category 2: Status Register Tests (8 tests) - KNOWN LIMITATION
All related to high-address status register access being intercepted by SCML2.

### Category 3: Other (1 test)
9. testDirected_InboundTlb_AllThreeTypes - TLB Sys In0 route=4 failure

## Next Actions
1. Fix address ranges in 7 transaction-failing tests
2. Skip or document the 8 status register tests
3. Investigate the TLB route=4 issue

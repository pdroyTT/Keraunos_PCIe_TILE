# SystemC IP Integration Guide for Synopsys Virtualizer VDK

## Quick Start

This guide helps you integrate custom SystemC IP components (like TLB modules) into Synopsys Virtualizer VDK.

### Documents Available

| # | Document | Purpose | Time |
|---|----------|---------|------|
| 0 | `00_INTEGRATION_OVERVIEW.md` | Complete overview and workflow | 15 min read |
| 1 | `01_PREPARE_SYSTEMC_IP.md` | Prepare IP with TLM interfaces | 2-8 hours |
| 2 | `02_PLATFORM_CREATOR_IMPORT.md` | Import into Platform Creator | 1-4 hours |
| 3-11 | Additional guides | TLM setup, VDK integration, debugging | Varies |

### For First-Time Users

**Start Here:**
1. Read `00_INTEGRATION_OVERVIEW.md` (15 minutes)
2. Follow `01_PREPARE_SYSTEMC_IP.md` step-by-step
3. Test your IP with the standalone testbench
4. Proceed to Platform Creator import

### For Experienced Users

**Quick Path:**
- Review checklist in `01_PREPARE_SYSTEMC_IP.md`
- Import to PCT using `02_PLATFORM_CREATOR_IMPORT.md`
- Skip to VDK integration

### Example: TLB Integration

The guides use **TLB (Translation Lookaside Buffer)** as the example IP:
- **Inbound TLB**: Device IOVA → Physical Address
- **Outbound TLB**: CPU Virtual Address → Physical Address

Both follow the same integration workflow.

---

## Tool Versions

- **Synopsys Virtualizer:** V-2024.03
- **SystemC:** 2.3.x
- **TLM:** 2.0
- **C++ Standard:** C++17

---

## Support

For issues:
1. Check `11_TROUBLESHOOTING.md` (when created)
2. Review Synopsys documentation in `/tools_vendor/synopsys/.../Documentation/`
3. Contact: vp_support@synopsys.com

---

**Created:** December 10, 2025  
**Location:** `/localdev/pdroy/keraunos_pcie_workspace/integration_guide/`

# SendBloom v1.0 Licensing Decision

**Decision date:** 2026-07-12  
**Decision owner:** Niko  
**Selected path:** Commercial JUCE distribution  
**Approval:** Product/legal direction approved in the Phase 27 autonomous discussion  
**Entitlement evidence:** `human_needed` — no JUCE account, invoice, subscription, or other covering entitlement was inspected in this run

## Decision

SendBloom intends to distribute under a valid commercial JUCE licence while retaining the repository's MIT licence for SendBloom-owned source and preserving complete third-party notices.

This document records the selected path, not proof that a commercial entitlement currently covers this release. RC0 promotion remains blocked until Niko confirms, with date and suitable private evidence reference, that the applicable JUCE commercial terms cover distribution of SendBloom 1.0.0-rc0. Do not commit invoices, account credentials, licence keys, or private contracts.

## Required confirmation

| Fact | Status | Evidence reference |
|---|---|---|
| Valid JUCE commercial entitlement held by the distributing entity | `human_needed` | Not supplied |
| Entitlement covers the version and release date used by SendBloom | `human_needed` | Not supplied |
| Repository/distribution may retain MIT for SendBloom-owned source | Blocked on the two facts above | This decision + `LICENSE` |
| Third-party notices complete | Automated check available | `docs/THIRD_PARTY_LICENSES.md`; `scripts/check-legal-metadata.sh` |

If commercial coverage cannot be confirmed, do not distribute under this decision. Re-plan the repository and distribution to meet JUCE's GPL obligations before release.

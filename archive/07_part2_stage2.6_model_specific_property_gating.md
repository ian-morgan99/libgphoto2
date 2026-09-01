# STAGE 2.6: Model-Specific Property Gating (Optional Refinement)

> **OBSOLETE:** Historical draft only. Use P2, P3, and P6 in [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md).
**Goal**: Ensure property get/set handlers check model capabilities before allowing access.

**Files to Modify**:
- `ptp2/config.c` – In property getters/setters, call `pentax_get_model_capabilities` and check if the property is supported for the model.

**Exact Changes**:
For each property getter/setter in `ptp2/config.c`, add:
```c
PentaxCameraCapabilities caps = pentax_get_model_capabilities(params);
if (!caps.some_feature_related_to_this_property) {
    return PTP_RC_InvalidParameter; // or specific error
}
```
before accessing the property.

**Exit Criteria**:
- Property get/set handlers correctly return errors for unsupported properties on a given model.
- Works correctly for at least two models.

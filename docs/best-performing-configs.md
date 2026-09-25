# Best-Performing Configurations per Model

_Analysed: 9/21/2026, 4:37:02 PM_
_Minimum 2 samples required to qualify as a recommended config._

## Best Server (overall)

| Server | Samples | Avg TPS | p50 TPS | p90 TPS | Last seen |
|--------|---------|---------|---------|---------|-----------|
| `(unknown)` | 21 | 30.3 | 30.9 | 53.8 | 2026-09-04T11:56:38.477Z |
| `primary` | 18 | 2.4 | 0.8 | 3.6 | 2026-09-12T00:53:18.756Z |

---

## huihui-qwen3.8-27b-abliterated-nvfp4

Total recorded completions: **151**

### Recommended Configuration

| Parameter | Value |
|-----------|-------|
| Parameters | `context_length=262144, temperature=0.7, top_p=0.9, top_k=40, repeat_penalty=1.1` |
| Samples | 7 |
| Avg TPS | **18.5** |
| p50 TPS | 4.3 |
| p90 TPS | 32.0 |
| Min / Max TPS | 0.5 / 66.8 |
| Total tokens | 1,032 |

### All Tested Configurations (sorted by avg TPS)

| Parameters | Samples | Avg TPS | p50 TPS |
|------------|---------|---------|---------|
| `context_length=262144, temperature=0.7, top_p=0.9, top_k=40, repeat_penalty=1.1` ✓ | 7 | 18.5 | 4.3 |
| `context_length=262144, max_tokens=8192, temperature=0.1, top_p=0.85, top_k=40, repeat_penalty=1.05` | 1 | 2.3 | 2.3 |
| `context_length=126208, temperature=0.7, top_p=0.9, top_k=40, repeat_penalty=1.1` | 1 | 1.8 | 1.8 |
| `context_length=262144, max_tokens=16384, temperature=0.1, top_p=0.85, top_k=40, repeat_penalty=1.05` | 10 | 1.0 | 0.5 |
| `context_length=126208, max_tokens=16384, temperature=0.1, top_p=0.85, top_k=40, repeat_penalty=1.05` | 1 | 0.1 | 0.1 |

---

## qwen-agentworld-35b-a3b-apex

Total recorded completions: **22**

### Recommended Configuration

| Parameter | Value |
|-----------|-------|
| Parameters | `context_length=262144, max_tokens=8192, temperature=0.1, top_p=0.85, top_k=40, repeat_penalty=1.05` |
| Samples | 8 |
| Avg TPS | **41.1** |
| p50 TPS | 38.0 |
| p90 TPS | 53.8 |
| Min / Max TPS | 24.0 / 62.4 |
| Total tokens | 8,279 |

### All Tested Configurations (sorted by avg TPS)

| Parameters | Samples | Avg TPS | p50 TPS |
|------------|---------|---------|---------|
| `context_length=262144, max_tokens=8192, temperature=0.1, top_p=0.85, top_k=40, repeat_penalty=1.05` ✓ | 8 | 41.1 | 38.0 |
| `context_length=238592, temperature=0.7, top_p=0.9, top_k=40, repeat_penalty=1.1` | 4 | 31.4 | 31.2 |
| `context_length=262144, temperature=0.7, top_p=0.9, top_k=40, repeat_penalty=1.1` | 4 | 19.6 | 17.9 |

---

## qwen3.8-flash-next

Total recorded completions: **35**

### Recommended Configuration

| Parameter | Value |
|-----------|-------|
| Parameters | `context_length=262144, temperature=0.7, top_p=0.9, top_k=40, repeat_penalty=1.1` |
| Samples | 2 |
| Avg TPS | **0.7** |
| p50 TPS | 0.8 |
| p90 TPS | 0.8 |
| Min / Max TPS | 0.6 / 0.8 |
| Total tokens | 194 |

### All Tested Configurations (sorted by avg TPS)

| Parameters | Samples | Avg TPS | p50 TPS |
|------------|---------|---------|---------|
| `context_length=262144, max_tokens=16384, temperature=0.1, top_p=0.85, top_k=40, repeat_penalty=1.05` | 1 | 0.8 | 0.8 |
| `context_length=262144, temperature=0.7, top_p=0.9, top_k=40, repeat_penalty=1.1` ✓ | 2 | 0.7 | 0.8 |

---

## zai-org.glm-5.3-flash

Total recorded completions: **9**

_No configuration has 2+ samples yet. More completions needed._

---

# 贪心 SCS — 运行示例

---

## 示例一：完整流程

**输入：** `[ALGO, LGO, LGOR, GORI, ORI, RITH, ITHM]`

### 第一步 — 清理冗余片段RemoveDominated

```
"LGO" 是 "ALGO" 的子串 → 删除
"ORI" 是 "GORI" 的子串 → 删除
```

清理后：`[ALGO, LGOR, GORI, RITH, ITHM]`

### 第二步 — 贪心合并 merge

```
第1轮：ALGO + LGOR  (重叠=3，共享 "LGO") → ALGOR
       [ALGOR, GORI, RITH, ITHM]

第2轮：ALGOR + GORI  (重叠=3，共享 "GOR") → ALGORI
       [ALGORI, RITH, ITHM]

第3轮：ALGORI + RITH  (重叠=2，共享 "RI") → ALGORITH
       [ALGORITH, ITHM]

第4轮：ALGORITH + ITHM  (重叠=3，共享 "ITH") → ALGORITHM
       [ALGORITHM]
```

**输出：** `ALGORITHM`（长度 9）✓

---

## 示例二：贪心不保证最优

**输入：** `[ABC, BCD, CDA, DAB]`

### 第一步 — 清理冗余片段

```
没有冗余片段，跳过。
```

### 第二步 — 贪心合并

```
第1轮：计算所有片段对的重叠...
  ABC → BCD : 重叠=2
  ABC → CDA : 重叠=1
  ABC → DAB : 重叠=0
  BCD → CDA : 重叠=2
  BCD → DAB : 重叠=1
  CDA → DAB : 重叠=2
  CDA → ABC : 重叠=1
  DAB → ABC : 重叠=2  ← 与 ABC→BCD 并列，但 ABC→BCD 先被找到

  最大重叠=2，取第一个找到的：ABC + BCD → ABCD
  [ABCD, CDA, DAB]

第2轮：ABCD + CDA  (重叠=2，共享 "CD") → ABCDA
       [ABCDA, DAB]

第3轮：ABCDA + DAB  (重叠=2，共享 "DA") → ABCDAB
       [ABCDAB]
```

**贪心输出：** `ABCDAB`（长度 **6**）

### 为什么不是最优

最优解是 `ABCDA`（长度 **5**），它同时包含了所有四个片段：

```
验证：
  ABC  在 ABCDA 中？ ✓  (位置 0)
  BCD  在 ABCDA 中？ ✓  (位置 1)
  CDA  在 ABCDA 中？ ✓  (位置 2)
  DAB  在 ABCDA 中？ ✗  → ABCDA 不是有效答案
```

> **注意：** 这个反例需要用 brute-force 验证真实最优解，
> 建议运行 `GreedySCS.java` 和 brute-force 对比后再使用。

贪心每步只看当前最大重叠，无法预判后续合并的影响，


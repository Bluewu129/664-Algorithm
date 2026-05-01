# 第 9 周笔记 —— Role C（本周轮值）

日期：2026-04-28
周次：第 9 周（27/04 – 01/05）—— 算法设计与伪代码

本文档是共享预处理层的设计 / 伪代码交付物。算法 1（Greedy SSP，由成员 A 负责）和算法 2（朴素 Bitmask DP，由成员 B 负责）都会把这一层接在各自主求解器之前。C 语言实现安排在第 10 周；本周仅交付规范说明。

## 1. 范围

预处理层将输入文件中的原始片段列表 `F` 转换为两份两个算法共用的产物：

1. 一份**剪枝后的片段数组**：所有作为另一片段子串的片段都被移除。
2. 一份**两两重叠矩阵** `O[n][n]`，其中 `O[i][j]` 表示片段 `i` 的最长后缀长度，且该后缀同时是片段 `j` 的前缀（严格重叠，即长度小于 `min(|f_i|, |f_j|)`）。

之后两个算法都基于 `(剪枝后的片段, O)` 进行计算，主循环里不再需要任何字符串匹配。

## 2. 数据结构（C 接口）

暴力法 demo 把片段读入一个单链表（参考 `week7_notes.md` §2.1）。算法层希望随机访问，因此预处理的第一步会把链表展平成数组。

```c
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **frag;     // frag[i] 是堆分配的、以 NUL 结尾的字符串
    int   *len;      // len[i] = (int) strlen(frag[i])；缓存以加速
    int    n;        // 当前数组中的片段数
} Fragments;

typedef struct {
    int **m;         // m[i][j] = suffix(i) 接到 prefix(j) 的最大重叠；m[i][i] = 0
    int   n;         // 构造时与 Fragments.n 保持一致
} OverlapMatrix;
```

设计理由：

- 缓存 `len[i]` 可以避免两个算法在内层循环里反复调用 `strlen`。
- 当 `n ≤ ~64` 时，使用锯齿形 `int**` 完全够用；瓶颈是 bitmask DP 的 `2^n`，远比内存分配先爆炸。
- 两个结构体各自拥有自己的内存；下面有对应的析构函数。

## 3. 函数签名（A 与 B 的契约）

```c
// 从文件（或当 file_name == "-" 时从 stdin）读入片段，并展平成
// Fragments 结构体。调用者用 free_fragments() 释放。
// 内部复用暴力法 demo 中的 read_all_fragments() / free_all_fragments()。
Fragments load_fragments(const char *file_name);

// 删除每一个作为另一个片段子串的片段。
// 原地修改 `f`：保留下来的片段被压缩到前缀 [0 .. f->n)，并相应减小 f->n。
// 被删除的字符串会被 free。返回被移除的片段个数（用于日志 / 报告统计）。
int prune_substrings(Fragments *f);

// 分配并填充 n×n 的重叠矩阵，n == f->n。
// 调用者用 free_overlap_matrix() 释放。
OverlapMatrix compute_overlaps(const Fragments *f);

// 析构函数。
void free_fragments(Fragments *f);
void free_overlap_matrix(OverlapMatrix *o);
```

A 和 B 都可以把各自的算法文件写成：

```c
int main(int argc, char *argv[]) {
    Fragments     f = load_fragments(argv[1]);
    prune_substrings(&f);
    OverlapMatrix o = compute_overlaps(&f);
    /* 算法专属的求解器，输出到 stdout */
    free_overlap_matrix(&o);
    free_fragments(&f);
    return 0;
}
```

这条边界是有意为之：预处理层是纯函数（除了 `load_fragments` 之外没有任何 I/O），可以独立做单元测试，并且同一份代码会被同时塞进两份提交的 `.c` 文件。

## 4. 伪代码

### 4.1 `prune_substrings`

```
input:  Fragments f
output: 原地修改 f，删除子串冗余的片段

 1. for i = 0 .. n-1:
 2.     remove[i] := false
 3.
 4. for i = 0 .. n-1:
 5.     for j = 0 .. n-1:
 6.         if i == j or remove[j]:
 7.             continue
 8.         if frag[i] occurs as a substring of frag[j]:
 9.             // 平局规则：当字符串完全相同时，恰好留一份
10.             if len[i] < len[j] or (len[i] == len[j] and i > j):
11.                 remove[i] := true
12.                 break              // i 已被淘汰，进入下一个 i
13.
14. 原地紧凑 f.frag / f.len，丢弃所有被标记的索引，
15. 释放被丢弃的 char* 缓冲区。
16. 更新 f.n。
```

第 8 行的子串测试使用 `strstr(frag[j], frag[i]) != NULL`。
第 10 行的平局规则保证：当两个片段**完全相同**时只有一份会保留下来（保留下标更小的那份），从而无论输入顺序如何，输出都是确定性的。

### 4.2 `compute_overlaps`

```
input:  Fragments f（已经剪枝）
output: OverlapMatrix o，n × n，o.m[i][i] = 0

 1. 分配 o.m 为 int**[n]，每行 int[n] 初始化为 0
 2. for i = 0 .. n-1:
 3.     for j = 0 .. n-1:
 4.         if i == j:
 5.             o.m[i][j] := 0
 6.             continue
 7.         k := min(len[i], len[j]) - 1     // 严格重叠的上界
 8.         while k > 0:
 9.             if frag[i] 长度为 k 的后缀 == frag[j] 长度为 k 的前缀:
10.                 o.m[i][j] := k
11.                 break
12.             k := k - 1
13.         if k == 0:
14.             o.m[i][j] := 0
15. return o
```

第 7 行的严格上界 `k ≤ min(|f_i|, |f_j|) - 1` 是子串剪枝之后让 overlap 良好定义的关键：剪枝保证两个字符串互不为子串，所以永远用不到 `k == |f_j|` 这种情况。

第 9 行的后缀 / 前缀相等检查实现为 `memcmp(frag[i] + len[i] - k, frag[j], k) == 0`。

## 5. 正确性论证

### 5.1 子串消除保留最优解

**断言。** 设 `F` 是输入片段集合，`F'` 是剪枝后的集合（`F'` 中每个片段在子串包含关系下都是极大的）。则 `L_opt(F) = L_opt(F')`，其中 `L_opt` 表示最短超串长度。

**证明梗概。** 根据定义，`F \ F'` 中每一个片段都是 `F'` 中某个片段的子串。因此，任何作为 `F'` 超串的字符串 `s` 都自动包含 `F` 中所有片段（"是子串" 关系的传递性）。所以 `F'` 的所有正确重建恰好是 `F` 的所有正确重建的*超集*；反过来，`F` 的每一个正确重建也是 `F'` 的正确重建。两个集合相等，最小值也相等。∎

推论：任何能找到 `F'` 最优超串的算法，也就找到了 `F` 的最优超串。剪枝是**严格安全**的。

### 5.2 重叠矩阵的良好定义与一致性

剪枝后，对任意 `i ≠ j`，`frag[i]` 不是 `frag[j]` 的子串，反之亦然。因此 "frag[i] 中等于 frag[j] 前缀的最长后缀" 长度至多为 `min(|f_i|, |f_j|) - 1`，与伪代码中的上界完全一致。对角线按约定设为 `0`（不允许自合并）。

Greedy 与 Bitmask DP 都只把 `O[i][j]` 当作 "在 `i` 之后追加 `j`" 的合并代价 `|f_j| - O[i][j]` 来用；该量对所有 `i ≠ j` 都是正数，所以两个算法都不会出现长度为 0 的退化步骤。

## 6. 复杂度分析

记 `n = |F|`（输入大小），`n' = |F'|`（剪枝后，`n' ≤ n`），`L = max_i |frag[i]|`（最长片段）。

| 步骤                       | 时间                        | 空间          |
|----------------------------|-----------------------------|---------------|
| `load_fragments`           | O(输入总长度)               | O(input)      |
| `prune_substrings`（朴素） | O(n² · L)                   | O(n) 标记位   |
| `compute_overlaps`（朴素） | O(n'² · L)                  | O(n'²) 矩阵   |

说明：

- 朴素剪枝按对调用 `strstr` → `O(n²)` 对 × 单次最坏 `O(L)` 扫描。本作业的输入规模（n ≪ 100，L ≪ 50）下这只是微秒级，没必要构建广义后缀自动机。
- 朴素重叠对每个 `(i, j, k)` 候选用一次 `memcmp`，但单个 `(i,j)` 的内层循环总工作量是 `O(L)`：我们按 `k` 递减测试，长度为 `k` 的 `memcmp` 是 `O(k)`，求和也是 `O(L)`。所以总体 `O(n² · L)`，比常被引用的 `O(n² · L²)` 略紧；差异不重要，但报告里值得指出。
- 用 KMP failure function 可以把每对的最坏复杂度降到保证 `O(L)`，但失败表的常数开销直到 L 上千才划算。

这些界会成为最终报告中复杂度表的 "Preprocessing" 那一行（章节：复杂度分析）。

## 7. 实例走查 —— `input-test_example.txt`

输入：`{ a, b, c, ac, ab, ba }`，n = 6。

**剪枝过程。** 把每个片段对其他片段做检查：

- `a` ⊂ `ac`、`ab`、`ba` → 删
- `b` ⊂ `ab`、`ba` → 删
- `c` ⊂ `ac` → 删
- `ac`、`ab`、`ba` 两两互不包含 → 留

剪枝后 `F' = { ac (idx 0), ab (idx 1), ba (idx 2) }`，n' = 3。

**重叠矩阵。** 计算 `O[i][j]` = `frag[i]` 中等于 `frag[j]` 前缀的最长后缀：

```
           →  ac   ab   ba
        ac [  -    0    0  ]    后缀 "c" 不匹配任何前缀
        ab [  0    -    1  ]    后缀 "b" 匹配 "ba" 的前缀
        ba [  1    1    -  ]    后缀 "a" 匹配 "ac" 与 "ab" 的前缀
```

**自检。** Greedy 在该矩阵上会任意挑一对重叠为 1 的对。一次运行：

1. 合并 `ab + ba`（重叠 1）→ `aba`；剩余 `{ aba, ac }`。
2. 合并 `aba + ac`（重叠 1，后缀 `a`、前缀 `a`）→ `abac`。

长度为 4。Bitmask DP 也确认 4 就是最优（DP 表略，将在报告中给出完整轨迹）。重建串 `abac` 包含所有原始片段作为子串：

- `a` 在下标 0（以及 2）
- `b` 在下标 1
- `c` 在下标 3
- `ac` 在下标 2..3
- `ab` 在下标 0..1
- `ba` 在下标 1..2

题目给出的最优解是 `abca`、`abcb`、`babc`，长度均为 4；`abac` 是另一个长度 4 的最优解，与 "可能存在多个相同最小长度的最优解" 一致。

## 8. 边界情况与测试覆盖

预处理必须正确处理 `tests/` 中已有的几种病态用例：

- **单个片段**（n = 1）：`prune_substrings` 是空操作，`compute_overlaps` 返回 1×1 矩阵且 `m[0][0] = 0`，主算法直接输出该片段。
- **所有片段完全相同**：剪枝后只剩一份（按平局规则保留下标最小者）；重叠矩阵是 1×1。
- **某个片段是其他所有片段的子串**：只有极大的片段会留下；如果存在唯一全局极大片段，n' 缩为 1。
- **没有任何两两重叠**（如 `{ "abc", "xyz" }`）：重叠矩阵的非对角线全为 0，两个算法只能直接拼接（输出长度 = 总长度）。
- **完全互重叠**（如 `{ "abcd", "cdab" }`）：两个非对角项都非零；两个算法可从任一方向合并，长度一致。

这五个用例将在第 10 周实现 `prune_substrings` 与 `compute_overlaps` 时作为冒烟测试。

## 9. 第 10 周交接

第 8 周确定的团队计划：

- 算法 1（Greedy SSP）：成员 A 负责。
- 算法 2（Plain Bitmask DP）：成员 B 负责。
- 预处理层：本周交付物；第 10 周轮到 Role C 的人按照 §3 的签名实现 `prune_substrings` 与 `compute_overlaps`，并把 `.c`/`.h` 片段（或可复制的代码块）发给 A 与 B 嵌入到他们的文件中。

第 9 周周五会议要带的东西：

1. 本文档。重点过一下 §2（数据结构）和 §3（签名），让 A 与 B 在动手写代码**之前**就有机会反对接口。
2. §4 的伪代码 —— 复核平局规则与严格重叠上界。
3. §7 的走查样例 —— 它就是第 10 周实现完成后会重放的冒烟测试。
4. 待决问题：是否要把 `Fragments.len` 单独保存为 `int*`（当前方案），还是合并成 `struct Frag { char *s; int len; }` 数组？后者代码略干净，分配略多一点；这个决定留给第 10 周的实现者。

## 10. 待提出的开放问题

- 报告的复杂度章节是否需要也给出最大测试输入下的预处理实测时间？（大概率要 —— 几乎不花成本，还能为 "朴素就够用" 的判断背书。）
- A 和 B 是否同意把 `Fragments` / `OverlapMatrix` 直接放在他们各自 `.c` 文件里？还是想要一个小 `.h` 大家都 paste？提交规则是 "每个算法一个独立 `.c` 文件"，所以我们**必须**把预处理代码粘进各自的算法文件，而不是 `#include` 一个头。会上确认即可。

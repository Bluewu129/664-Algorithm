# 贪心最短公共超串算法（Greedy SCS）

**目标：** 找到一个最短的字符串，使得每个输入片段都是它的子串。
算法思路：每次把重叠最多的两个片段合并，重复直到只剩一个字符串。

| 属性 | 说明 |
|------|------|
| 时间复杂度 | O(n³ × k)，n = 片段数量，k = 最长片段长度 |
| 最优性 | **不保证最优** — 贪心可能错过全局最优解 |

---

## 主函数：GreedySCS(fragments)

```text
// 第一步：清理冗余片段
// 如果某个片段已经被另一个片段完整包含，直接删掉它
// 例如 ["abcd", "bc"] → 删掉 "bc"，因为它已经在 "abcd" 里了
RemoveDominated(fragments)

// 第二步：反复合并，直到只剩一个片段
WHILE fragments.size > 1 DO

    // 初始化本轮的"最佳合并方案"
    // maxOverlap 设为 -1 而不是 0，是为了保证即使重叠为 0 也能正常赋值
    maxOverlap = -1
    bestFirst  = null   // 本轮选中的第一个片段
    bestSecond = null   // 本轮选中的第二个片段
    bestMerged = null   // 合并后的结果

    // 枚举所有片段对 (A, B)，找重叠最大的那一对
    FOR each A in fragments DO
        FOR each B in fragments DO

            // 跳过自己和自己比较
            IF A is B THEN
                CONTINUE

            // 计算 A 的尾部和 B 的头部最长能重合多少
            overlap = CalculateOverlap(A, B)

            // 如果比当前最大重叠更大，更新最佳方案
            IF overlap > maxOverlap THEN
                maxOverlap = overlap
                bestFirst  = A
                bestSecond = B
                bestMerged = Merge(A, B, overlap)

    // 用合并结果替换原来的两个片段
    REMOVE bestFirst  from fragments
    REMOVE bestSecond from fragments
    ADD    bestMerged to fragments

// 第三步：循环结束后只剩一个片段，就是答案
RETURN the only string left in fragments
```

---

## RemoveDominated(fragments)

**作用：** 删除已经被其他片段包含的冗余片段，避免浪费合并次数。

```text
toRemove = empty set

FOR each A in fragments DO
    FOR each B in fragments DO

        // 如果 A 是 B 的子串（且不是同一个），A 就是冗余的
        IF A is not B AND A is a substring of B THEN
            ADD A to toRemove
            BREAK   // A 已经确认冗余，不需要继续检查

// 统一删除所有冗余片段
REMOVE all strings in toRemove from fragments
```

> 例：输入 `["abcd", "bc", "cdef"]`
> `"bc"` 是 `"abcd"` 的子串 → 删掉
> 剩下 `["abcd", "cdef"]`

---

## CalculateOverlap(A, B)

**作用：** 计算 A 的尾部和 B 的头部最长能重合多少个字符。
从最长可能的重叠开始试，找到就立刻返回。

```text
// 重叠长度不可能超过两个片段中较短的那个
maxK = min(length(A), length(B))

// 从最大值往下试，找到第一个匹配就返回
FOR k = maxK DOWN TO 1 DO
    IF suffix of A with length k == prefix of B with length k THEN
        RETURN k

// 没有任何重叠
RETURN 0
```

> 例：`CalculateOverlap("abcd", "cdef")`
> k=4: `"abcd"` vs `"cdef"` → 不匹配
> k=3: `"bcd"`  vs `"cde"` → 不匹配
> k=2: `"cd"`   vs `"cd"`  → 匹配，返回 2

---

## Merge(A, B, overlap)

**作用：** 把 A 和 B 合并成一个字符串，去掉重复的重叠部分。

```text
// 保留 A 的全部，再拼上 B 去掉前 overlap 个字符后的部分
RETURN A + B[overlap:]
```

> 例：`Merge("abcd", "cdef", 2)`
> B 去掉前 2 个字符 → `"ef"`
> 结果 = `"abcd"` + `"ef"` = `"abcdef"`

---

## 完整运行示例

输入：`["abcd", "cdef", "efgh"]`

**第 1 轮：**

| 片段对 (A → B) | 重叠长度 |
|----------------|----------|
| abcd → cdef | 2 |
| abcd → efgh | 0 |
| cdef → efgh | 2 |
| 其余方向 | 0 |

重叠最大的是 `abcd + cdef`（取第一个找到的），合并为 `"abcdef"`
当前列表：`["abcdef", "efgh"]`

**第 2 轮：**

| 片段对 (A → B) | 重叠长度 |
|----------------|----------|
| abcdef → efgh | 2 |
| efgh → abcdef | 0 |

合并为 `"abcdefgh"`，列表只剩一个元素，返回结果。

**最终输出：** `"abcdefgh"`（长度 8）

---

## 为什么贪心不保证最优

反例：`["ABC", "BCD", "CDA", "DAB"]`

- 贪心输出：`"ABCDAB"`（长度 6）
- 最优解：`"ABCDA"`（长度 5）

贪心每步只看当前最大重叠，但局部最优的选择可能阻断了更好的全局排列。

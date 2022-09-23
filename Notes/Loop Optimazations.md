参考[wiki](https://en.wikipedia.org/wiki/Optimizing_compiler)

# <center> Loop optimizations
## 1&emsp;Loop-invariant code motion
[循环不变代码](https://en.wikipedia.org/wiki/Loop-invariant_code_motion)可被移出循环体而不影响程序的语义。

> 通常使用 ***[reaching definitions analysis（到达定义分析）](https://en.wikipedia.org/wiki/Reaching_definition)*** 检测语句或表达式是否为循环不变量。如，若表达式中所有操作数的到达定义都在循环外，那么该表达式可被移到循环。

reaching definitions是一种数据流分析，静态地确定哪些定义可以到达代码中的给定位置。例如：
```
//d2是d3的到达定义，d1不是d3的到达定义
d1 : y := 3
 d2 : y := 4
 d3 : x := y
```

### Example 
```
//循环不变量 x = y+z、x*x
int i = 0;
while ( i < n ) {
    x = y + z;
    a[i] = 6 * i + x * x;
    ++i;
}

//optimize
int i = 0;
x = y + z;
int const t1 = x * x;
while( i < n ) {
        a[i] = 6 * i + t1;
        ++i;
} 
```

## 2&emsp;[loop inversion](https://en.wikipedia.org/wiki/Loop_inversion)
将 `while` 等价转换为 `if` 嵌套 `do { } while。
### Example
```
//source code
  int i, a[100];
  i = 0;
  while (i < 100) {
    a[i] = 0;
    i++;
  }

//After loop inversion
int i, a[100];
  i = 0;
  if (i < 100) {
    do {
      a[i] = 0;
      i++;
    } while (i < 100);
  }
```

## 3&emsp;loop fusion（循环合并）
参考[知乎](https://zhuanlan.zhihu.com/p/315041435)
### Example：循环合并的合法性
```
DO I = 1, N
  S1: A(I) = B(I) + C1
ENDDO
DO I = 1, N
  S2: D(I) = A(I+1) + C2
ENDDO
```
合并上述两个循环是不合法的，因为在合并前S2中的A(I+1)会读到被第一个循环写入的值，而在合并后A(I+1)则会读入修改之前的值，于是数据依赖被破坏了，不合法的代码如下：
```
DO I = 1, N
  S1: A(I) = B(I) + C1
  S2: D(I) = A(I+1) + C2
ENDDO
```
解决：循环对齐（loop alignment）。首先把第二个循环的index加一，这样可以把原本的A(I+1)变成A(I)，这样假如合并的话S2将会读到新的值。
```
DO I = 1, N
  S1: A(I) = B(I) + C1
ENDDO
DO I = 2, N+1
  S2: D(I-1) = A(I) + C2
ENDDO
```
再考虑对齐循环边界，即只合并公共循环部分，得到：
```
A(1) = B(1) + C1
DO I = 2, N
  S1: A(I) = B(I) + C1
  S2: D(I-1) = A(I) + C2
ENDDO
D(N) = A(N+1) + C2
```
> 循环合并本质上是把**第二个循环提前执行**了，到底能提前多少取决于两个循环之间的数据依赖。保存原本数据依赖关系的方法是***循环对齐（loop alignment）***。循环对齐本质上是在执行S1(I)后执行S2(I-k)，其中k是对齐的offset。**只要在对齐后保证对于同数组的访问下标S2的都等于或小于S1的，原有的数据依赖则能保存。**

例如，下面的例子直接合并是合法的，因为没有改变数据依赖关系。
```
DO I = 2, N
  S1: A(I) = B(I) + C1
ENDDO
DO I = 2, N
  S2: D(I) = A(I-1) + C2
ENDDO
```

## 4&emsp;[induction variables（归纳变量）](https://en.wikipedia.org/wiki/Induction_variable)
**归纳变量**是指增加或减少**固定量**的变量，或者是另一个归纳变量的线性函数。
 
### Example：application to strength reduction
```
// i 和 j 都是归纳变量
for (i = 0; i < 10; ++i) {
    j = 17 * i;
}

//optimize： Δj = 17 * (Δi) = 17 * 1 = 17
j = -17;
for (i = 0; i < 10; ++i) {
    j = j + 17;
}
```

### Example：application to reduce register pressure
```
// 减少变量
extern int sum;
int foo(int n) {
    int i, j;
    j = 5;
    for (i = 0; i < n; ++i) {
        j += 2;
        sum += j;
    }
    return sum;
}

//optimize
extern int sum;
int foo(int n) {
    int i;
    for (i = 0; i < n; ++i) {
        sum += 5 + 2 * (i + 1);
    }
    return sum;
}
```

## 5&emsp;[Strength reduction](https://en.wikipedia.org/wiki/Strength_reduction)
Strength reduction，即，将”强“操作等价替换为”弱“操作。如，循环中用加法替换乘法，乘法替换幂运算。Strength reduction**作用于**存在循环不变量和归纳变量的表达式。

## 6&emsp;[loop pipelining（循环流水线）](https://en.wikipedia.org/wiki/Software_pipelining)
指令之间的依赖关系(instructions dependencies) 会限制 CPU 的运行速度，比如有两条指令 A 和 B，而 B 指令的操作数是从 A 指令的结果拿的，那么 B 就依赖于 A 了。最简单的案例就如 c[i] = a[i] + b[i]。
而loop pipelining 可以打破这种依赖的链路，实际上就是把每次循环里的**操作分离**。
loop pipelining通常与**循环展开**相结合。

### Example：循环内语句有数据依赖关系
```
//优化分离方式：计算上一次的结果、加载下一次的数据。
for (int i = 0; i < n; i++) {
   val_a = load(a + i);
   val_b = load(b + i);
   val_c = add(val_a, val_b);
   store(val_c, c + i);
}

//optimize：将循环体中的语句顺序倒转
val_a = load(a + 0);  
val_b = load(b + 0);
val_c = add(val_a, val_b);

val_a = load(a + 1);
val_b = load(b + 1);

for (int i = 0; i < n - 2; i++) {
    store(val_c, c + i);
    val_c = add(val_a, val_b);
    val_a = load(a + i + 2);
    val_b = load(b + i + 2);
}

store(val_c, n - 2);

val_c = add(val_a, val_b);
store(val_c, n - 1);

```

### Example：循环内操作之间无数据依赖关系，但以某一顺序操作相同变量
```
//A、B、C之间无数据依赖关系，但顺序地对 i 进行操作
//即，i 确定时，操作顺序为A，B，C；i 不同时，同一操作之间无先后顺序，如操作A(2)可在A(1)之前

for i = 1 to bignumber
  A(i)
  B(i)
  C(i)
end

//optimize：利用循环展开，注意通常迭代总数能被展开的迭代次数整除。

for i = 1 to (bignumber - 2) step 3
  A(i)
  A(i+1)
  A(i+2)
  B(i)
  B(i+1)
  B(i+2)
  C(i)
  C(i+1)
  C(i+2)
end
```

## 7&emsp;[loop unrolling（循环展开）](https://en.wikipedia.org/wiki/Loop_unrolling)
循环展开的**目标**是通过减少或消除控制循环的指令来提高程序的速度，即，**降低循环体的开销**。

### Example
```
//source code
 int x;
 for (x = 0; x < 100; x++)
 {
     delete(x);
 }

//after loop unrolling
int x; 
 for (x = 0; x < 100; x += 5 )
 {
     delete(x);
     delete(x + 1);
     delete(x + 2);
     delete(x + 3);
     delete(x + 4);
 }
```

### Example
```
// 表达式 i / 2 使得每两次循环 变量 index 值相同 
for (int i = 0; i < n; i++) {
    index = i / 2;
    b_val = load(b + index);
    store(a + i, b_val);
}

// After loop unrolling：展开的迭代次数为2，有效地减少了 load 的次数
for (int i = 0; i < n; ) {
    index = i / 2;
    b_val = load(b + index);
    store(a + i, b_val);
    i++;

    store(a + i, b_val);
    i++;
}
```

## 8&emsp;[loop unswitching](https://en.wikipedia.org/wiki/Loop_unswitching)
loop unswitching 是指，对循环中的 `if` 语句，**为判断条件得到的不同分支操作，创建各自的循环体版本**。

### Example
```
// source code
  int i, w, x[1000], y[1000];
  for (i = 0; i < 1000; i++) {
    x[i] += y[i];
    if (w)
      y[i] = 0;
  }

//After loop unswitching：根据 if(w) 的不同分支，创建两个循环体
int i, w, x[1000], y[1000];
  if (w) {
    for (i = 0; i < 1000; i++) {
      x[i] += y[i];
      y[i] = 0;
    }
  } else {
    for (i = 0; i < 1000; i++) {
      x[i] += y[i];
    }
  }
```
                         
### 第一种：循环不变量和循环次数作为condition
1. 循环不变量循环体内外都是一致的
2. 循环次数在等价变换之后只要满足某种线性关系就可。

### 第二种：在循环体内部加入condition
1. 找循环体内部某些操作满足的线性关系：>/</=/整除…… 将几种不同的数据类型抽象成比较系统的matchcer方法

### 直接在一个文件里面写出等价变换前后的两种，对比其结果应该一致（直接通过CPAchecker result判断）（系统general）
### ？？？需要两个文件结果对比才能知道结果，事先不知道具体的结果？？？

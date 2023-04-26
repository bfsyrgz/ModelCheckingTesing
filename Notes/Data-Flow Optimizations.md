参考[知乎](https://zhuanlan.zhihu.com/p/381490718)

# Data-Flow Optimizations
## 1&emsp;[Common subexpression elimination](https://en.wikipedia.org/wiki/Common_subexpression_elimination)（公共子表达式消除，CSE）

> 属于**正向数据流分析**

公共子表达式消除是基于[可用表达式(available expression)](https://en.wikipedia.org/wiki/Available_expression)分析。可用表达式是指在程序某点不需重复计算的表达式，这要求表达式的操作数在从该表达式的出现到该程序点的任何路径上都未被修改。

因此，如果一个表达式 E 在 p 点之前已经计算过了，并且从该计算位置到 p 点之间的所有路径中 E 的操作数（指变量）都没有发生变化，那么 E 就是 p 点的公共子表达式。
### Example
```
//source code
a = b * c + g;
d = b * c * e;

//after optimization: b*c 是公共子表达式
tmp = b * c;
a = tmp + g;
d = tmp * e;
```

## 2&emsp;[Constant folding and propagation](https://en.wikipedia.org/wiki/Constant_folding)（常量折叠和常量传播）

> 都属于**正向数据流分析**

**常量折叠**是指，有些计算在编译时就能计算出来，那么这种我们就可以直接做，不需要推迟到运行时去做。

常量折叠不仅适用于数字，也适用于常量字符串连接，如`"abc" + "efg"` 可替换为 `"abcegf"`。  

### Example：Constant folding
```
//source code
int main(int argc,char **argv){
    int a = 1;
    int b = 2;
    int x = a + b;
    printf("%d\n", x);
    return 0;
}

//after folding
int main(int argc,char **argv){
    int x = 1 + 2;
    printf("%d\n", x);
    return 0;
}
```

**常量传播**是在编译时替换表达式中已知常量的值的过程。

常量传播是基于[reaching definition](https://en.wikipedia.org/wiki/Reaching_definition)分析的结果。如果所有变量的到达定义都被赋以相同的常量，那么该变量具有常量值，并可被该常量值替换。 

### Example：常量传播
```
//source code
int x = 14;
  int y = 7 - x / 2;
  return y * (28 / x + 2);
  
//传播 x 得到：
  int x = 14;
  int y = 7 - 14 / 2;
  return y * (28 / 14 + 2);
  
//继续传播 y 得到：
  int x = 14;
  int y = 0;
  return 0;
```
> 常量折叠和常量传播通常一起使用。

## 3&emsp;[Dead-store elimination](https://en.wikipedia.org/wiki/Dead_store)

> 属于**逆向数据流分析**

**Dead store**是指被赋值但未被任何后续指令读取的局部变量。

扩展：**无用代码消除**指的是永远不能被执行到的代码或者没有任何意义的代码会被清除掉，比如return之后的语句，变量自己给自己赋值等等。

### Example: Dead store
```
// DeadStoreExample.java
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class DeadStoreExample {
    public static void main(String[] args) {
        List<String> list = new ArrayList<String>(); // This is a Dead Store, as the ArrayList is never read. 
        list = getList();
        System.out.println(list);
    }

    private static List<String> getList() {
        return new ArrayList<String>(Arrays.asList("Hello"));
    }
}
```

### Example：无用代码消除
```
//source code
int main(int argc,char **argv){
    int x = 1;
    int x = x;
    std::cout<<x<<std::endl;
    return 0;
}

//变量 x 的自我赋值显然是无用代码，可消除
int main(int argc,char **argv){
    int x = 1;
    std::cout<<x<<std::endl;
    return 0;
}
```

## 4&emsp;[Alias classification and pointer analysis](https://en.wikipedia.org/wiki/Aliasing_(computing)#Conflicts_with_optimization)

**内存别名**是指，不同变量（两个及以上的指针）指向同一物理地址的现象。

### Example：缓冲区溢出

```
int main()
{
  int arr[2] = { 1, 2 };
  int i=10;

  /* Write beyond the end of arr. Undefined behaviour in standard C, will write to i in some implementations. */
  arr[2] = 20;

  printf("element 0: %d \t", arr[0]); // outputs 1
  printf("element 1: %d \t", arr[1]); // outputs 2
  printf("element 2: %d \t", arr[2]); // outputs 20, if aliasing occurred
  printf("i: %d \t\t", i); // might also output 20, not 10, because of aliasing, but the compiler might have i stored in a register and print 10
  /* arr size is still 2. */
  printf("arr size: %d \n", sizeof(arr) / sizeof(int));
}
```

上例中，`arr[2] = 20` 可能会覆盖内存中 `i` 的值。数组是一个连续的内存块，数组元素通过基地址加上偏移量与元素大小之积来引用，由于 C 没有边界检查，所以可以在数组边界之外进行索引和寻址。上例中，数组 `arr` 和局部变量 `i` 在内存中可能相邻存放，虽然数组的大小为2，但是arr[2]内存位置可以引用，若该位置也是 i 的内存位置，那么 i 的值就会被覆盖。

### Example：别名指针

```
//source code
void accumulate(int* source, int len, int* target) {
    for (int i = 0; i < len; i++) {
        *target += source[i];
    }
}

//after：不用在每次循环中都读取并写入 target 指向内存一次
void accumulate_improve(int* source, int len, int* target) {
    int acc = *target;
    for (int i = 0; i < len; i++) {
        acc += source[i];
    }
    *target = acc;
}
```

### Example：restrict用法(所有修改该指针所指向内容的操作全部都是基于(base on)该指针的，即不存在其它进行修改操作的途径)(-std=c99 -O2)

```
//source code
int sum(int *x, int *y)
{
	*x=3;
	*y=4;
	return *x + *y; //return 8;
}


int main()
{
	int x=2;
	printf("%d\n", sum(&x,&x));
	return 0;
}

//restrict限制了每个内存空间只能被一个指针修改
int sum(int _restrict *x, int _restrict *y)
{
	*x=3;
	*y=4;
	return *x + *y; //return 7;
}


int main()
{
	int x=2;
	printf("%d\n", sum(&x,&x));
	return 0;
}
```



### 与优化的冲突

优化器通常必须对变量做出保守的假设。

#### 1.&ensp;常量优化
例如，通常一个值被赋给一个变量（如 x = 5）时，会允许某些优化（例如constant propagation）。但是，当一个值被赋给另一个变量时，就不一定能进行优化（例如，在 C 中，`*y = 10`），这是因为若 `*y` 是变量 `x` 的别名（例如，在 `x = 5` 之后有 `y = &x`），那么当对 `*y` 赋值时，x 的值也会改变，使得传播的信息 x = 5 是错误的。

因此，如果有关于指针的信息，常量传播过程可以进行如下查询：可以 `*y` 是 `x` 的别名，`x = 5` 不能安全传播，反之可以。

#### 2.&ensp;代码顺序
受别名影响的另一个优化是代码重新排序。如果 `x` 没有别名 `*y`，那么使用或改变 x 值的代码都可以移到 `*y`之前。

## 5&emsp;[Induction variable（归纳变量）](https://github.com/bfsyrgz/ModelChecking_CompilerOptimization/blob/main/Notes/Loop%20Optimazations.md#4induction-variables%E5%BD%92%E7%BA%B3%E5%8F%98%E9%87%8F)
<img width="929" alt="Other strength reduction operations" src="https://user-images.githubusercontent.com/71619681/208819069-bc30ecbe-49d0-4d93-87dd-c5794334ba52.png">

### Example：指数eliminate乘除 ends加减

```
// our modified pow function that raises a to the power of b
// without using multiplication or division 
// return a^n
function modPow(a, n) {
  
  // convert a to positive number
  var answer = Math.abs(a);
  
  // store exponent for later use
  var exp = n;
  
  // loop n times
  while (n > 1) {
    
    // add the previous added number n times
    // e.g. 4^3 = 4 * 4 * 4
    //      4*4 = 4 + 4 + 4 + 4 = 16
    //     16*4 = 16 + 16 + 16 + 16 = 64
    var added = 0;
    for (var i = 0; i < Math.abs(a); i++) { added += answer; }
    answer = added;
    n--;
    
  }
  
  // if a was negative determine if the answer will be
  // positive or negative based on the original exponent
  // e.g. pow(-4, 3) = (-4)^3 = -64
  return (a < 0 && exp % 2 === 1) ? -answer : answer;
  
}

modPow(2, 10); 

```


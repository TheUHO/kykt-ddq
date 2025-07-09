[toc]

### chroma/qdp++ 数据结构 data layout 
##### Introduction
###### 1. Lattice Type  
```
// ref: qdpxx/include/qdp_scalarsite_defs.h
typedef OLattice< PScalar< PColorMatrix< RComplex<REAL>, Nc> > > LatticeColorMatrix;
typedef OLattice< PSpinVector< PColorVector< RComplex<REAL>, Nc>, Ns> > LatticeFermion;
typedef OLattice< PSpinMatrix< PColorMatrix< RComplex<REAL>, Nc>, Ns> > LatticePropagator;
...
```

```C++
// ref: qdpxx/include/qdp_reality.h
// 与C++的std::complex相似
template<class T> class RComplex
{
public:
  T& real() {return re;}
  const T& real() const {return re;}
  T& imag() {return im;}
  const T& imag() const {return im;}
private:
  T re;
  T im;
}
```

###### 2. QDP_USE_CB2_LAYOUT
以最常用(也是QDP++ 默认)的**奇偶排布**(`QDP_USE_CB2_LAYOUT`)为例说明：
```bash
 if( ${qdpLayout} STREQUAL "cb2")
  message( STATUS "QDP++: Enabling CB2 (4D Checkerboard) Layout" )
  set(QDP_USE_CB2_LAYOUT ON)
```
###### 3. 符号表示说明
- 格子大小: $nrow={X,Y,Z,T}$
- 格点坐标: $coord = \{x,y,z,t\}$
- 方向: $\mu = \{0,1,2,3\}$
- Spin: $s$
- $Ns=4$
- Color: $c$
- $Nc=3$

##### 1. `LatticeFermion`
###### 定义:  
  `typedef OLattice< PSpinVector< PColorVector< RComplex<REAL>, Nc>, Ns> > LatticeFermion;`

###### 说明:  
- 这是一个`T`类型元素的 `OLattice<T>`类; 可以通过`getF()`获得首个元素的地址
- `T = PSpinVector< PColorVector< RComplex<REAL>, Nc>, Ns>`, 一个格点上的Spin和Color
- 注意, 这里是$SpinVector \otimes ColorVector:= Vec[N_s] \otimes Vec[N_c]$ 
- 实际上等价于 `RComplex<Real> [Ns][Nc]`; 

###### 计算方式:  
- X维度的一半: $X_h = X/2$  
- 体积: $V=X*Y*Z*T$  
- 体积的一半: $V_h= X_h*Y*Z*T$  
- 坐标奇偶性: $cb=(x+y+z+t)\%2$
- 线性坐标索引: $i_h = x+ y*X_h + z*X_h*Y + t*X_h*Y*Z$  
- 奇偶坐标索引: $i_{eo} = i_h + cb * V_h$
- 排布方式: $i_{eo}*N_s*N_c + s*N_c + c$
- $[x+ y*X_h + z*X_h*Y + t*X_h*Y*Z + ((x+y+z+t)\%2)*((X/2)*Y*Z*T)]*N_s*N_c+ s*N_c + c$
- QDP访问方式: ` Rcomplex<Real> e = phi.elem(Ieo).elem(s).elem(c)`

##### 2. `LatticePropagator`
###### 定义:  
  `typedef OLattice< PSpinMatrix< PColorMatrix< RComplex<REAL>, Nc>, Ns> > LatticePropagator;`

###### 说明:  
- 这是一个`T`类型元素的 `OLattice<T>`类; 可以通过`getF()`获得首个元素的地址
- `T = PSpinMatrix< PColorMatrix< RComplex<REAL>, Nc>, Ns` 
- 这里是$PSpinMatrix \otimes PColorMatrix:= Mat[N_s*N_s] \otimes Mat[N_c*N_c]$ 
- 实际上等价于 `RComplex<Real> [Ns][Ns][Nc][Nc]`; 

###### 计算方式:  
- X维度的一半: $X_h = X/2$  
- 体积: $V=X*Y*Z*T$  
- 体积的一半: $V_h= X_h*Y*Z*T$
- 坐标奇偶性: $cb=(x+y+z+t)\%2 = {0或1}$ 
- 线性坐标索引: $i_h = x+ y*X_h + z*X_h*Y + t*X_h*Y*Z$  
- 奇偶坐标索引: $i_{eo} = i_h + cb * V_h$
- 排布方式: $[i_{eo}*N_s*N_s*N_c*N_c] + [(s_i*N_s +s_j) *N_c*N_c] + [c_i * N_c + c_j]$
- QDP访问方式: ` Rcomplex<Real> e = phi.elem(Ieo).elem(s_i,s_j).elem(c_i,c_j)`


##### 2. Gauge: `multi1d<LatticeColorMatrix>`
###### 定义:  
```C++
  // LatticeColorMatrix
  typedef OLattice< PScalar< PColorMatrix< RComplex<REAL>, Nc> > > LatticeColorMatrix;
  // Nd个LatticeColorMatrix构成一维数组;
  multi1d<LatticeColorMatrix> U(Nd);
```
###### 说明:  
- 这是一个`LatticeColorMatrix`类型元素的 `muti1d`类(一维数组); 可以通过`[i]`方式访问第i个元素
- `LatticeColorMatrix` 是一个`T`类型元素的 `OLattice<T>`类,
- `T = PScalar< PColorMatrix< RComplex<REAL>, Nc> > ` 
- 实际上等价于 `RComplex<Real> [Nc][Nc]`; 

###### 计算方式:  
- X维度的一半: $X_h = X/2$  
- 体积: $V=X*Y*Z*T$  
- 体积的一半: $V_h= X_h*Y*Z*T$  
- 坐标奇偶性: $cb=(x+y+z+t)\%2$ 
- 线性坐标索引: $i_h = x+ y*X_h + z*X_h*Y + t*X_h*Y*Z$  
- 奇偶坐标索引: $i_{eo} = i_h + cb * V_h$
- 排布方式: $[\mu*V*Nc*N_c] + [i_{eo}*N_c*N_c] +  [c_i * N_c + c_j]$
- QDP访问方式: ` Rcomplex<Real> e = U[mu].elem(i_eo).elem().elem(c_i,c_j)`

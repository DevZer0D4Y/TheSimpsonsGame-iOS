/*    0.0 */       exec
/*    7   */          vfetch_full r5.xyz_, r0.x, vf0, DataFormat=FMT_32_32_32_FLOAT, Stride=12, Signed=true, NumFormat=integer, PrefetchCount=7
/*    8   */          vfetch_mini r8.xyz_, Offset=3, DataFormat=FMT_2_10_10_10, Signed=true
/*    9   */          vfetch_mini r11.xy__, Offset=4, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*   10   */          vfetch_mini r4.wzyx, Offset=6, DataFormat=FMT_8_8_8_8, NumFormat=integer
/*   11   */          vfetch_full r9, r0.x, vf0, Offset=7, DataFormat=FMT_32_32_32_32_FLOAT, Stride=12, Signed=true, NumFormat=integer, PrefetchCount=5
/*   12   */          vfetch_mini r1.zyxw, Offset=11, DataFormat=FMT_8_8_8_8
/*    0.1 */       exec
/*   13   */          vfetch_full r10.xyz_, r0.x, vf1, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   14   */          vfetch_full r3.xyz_, r0.x, vf2, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   15   */          vfetch_full r2.xyz_, r0.x, vf3, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   16   */          vfetch_full r0._xyz, r0.x, vf4, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   17   */          vfetch_full r6.xyz_, r0.x, vf5, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   18   */          vfetch_full r7.xyz_, r0.x, vf6, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*    1.0 */       exec    // PredicateClean=false
/*   19   */          serialize
                      sgt r0.x___, c39.wwww, c255.zzzz
/*   20   */          setp_ne r0._, r0.x
/*    1.1 */ (!p0) jmp L5
/*    2.0 */       exec
/*   21   */          mad r5.xyz_, r10.xyzz, c38.xxxx, r5.xyzz
/*   22   */          mad r3.xyz_, r3.xyzz, c38.yyyy, r5.xyzz
/*   23   */          mad r2.xyz_, r2.xyzz, c38.zzzz, r3.xyzz
/*   24   */          mad r0.xyz_, r0.yzww, c38.wwww, r2.xyzz
/*   25   */          mad r0.xyz_, r6.xyzz, c39.xxxx, r0.xyzz
/*   26   */          mad r5.xyz_, r7.xyzz, c39.yyyy, r0.xyzz
                label L5
/*    2.1 */       alloc position
/*    3.0 */       exec
/*   27   */          cndeq r5, c255.xxxy, r5.zxyy, c255.yyyy
/*   28   */          mul r6, r4, c255.wwww
/*   29   */          maxas r0._, r6.xx
/*   30   */          mul r3, r9.wwww, c[54+a0].wxyz
/*   31   */          mul r2, r9.wwww, c[52+a0].wxyz
/*   32   */          mul r0, r9.wwww, c[53+a0].wxyz
              +       maxas r0._, r6.yy
/*    3.1 */       exec
/*   33   */          mad r0, r9.zzzz, c[53+a0].wxyz, r0
/*   34   */          mad r2, r9.zzzz, c[52+a0].wxyz, r2
/*   35   */          mad r3, r9.zzzz, c[54+a0].wxyz, r3
/*   36   */          maxas r0._, r6.zz
/*   37   */          mad r3, r9.yyyy, c[54+a0].wxyz, r3
/*   38   */          mad r4, r9.yyyy, c[52+a0].wxyz, r2
/*    4.0 */       exec
/*   39   */          mad r0, r9.yyyy, c[53+a0].wxyz, r0
/*   40   */          sge r2.___w, r_abs[0].xxxx, c255.xxxx
              +       maxas r0._, r6.ww
/*   41   */          mad r0, r9.xxxx, c[53+a0].wxyz, r0
/*   42   */          mad r4, r9.xxxx, c[52+a0].wxyz, r4
/*   43   */          mad r3, r9.xxxx, c[54+a0].wxyz, r3
/*   44   */          dp4 r2.x___, r3.wyzx, r5
/*    4.1 */       exec
/*   45   */          dp4 r2._y__, r4.wyzx, r5
/*   46   */          dp4 r2.__z_, r0.wyzx, r5
/*   47   */          dp4 oPos.x___, c0.zxyw, r2
/*   48   */          dp4 oPos._y__, c1.zxyw, r2
/*   49   */          dp4 oPos.__z_, c2.zxyw, r2
/*   50   */          dp4 oPos.___w, c3.zxyw, r2
/*    5.0 */       alloc interpolators
/*    5.1 */       exec
/*   51   */          dp3 r0._y__, r0.wyzz, r8.zxyy
/*   52   */          dp3 r0.__z_, r4.wyzz, r8.zxyy
/*   53   */          dp3 r0.___w, r3.wyzz, r8.zxyy
/*   54   */          dp3 r0.x___, r0.wzyy, r0.wzyy
/*   55   */          max o0.xy__, r11.xyyy, r11.xyyy
/*   56   */          max o3, r1, r1
/*    6.0 */       exec
/*   57   */          dp4 o2.x___, c12.zxyw, r2
/*   58   */          dp4 o2._y__, c13.zxyw, r2
/*   59   */          dp4 o2.__z_, c14.zxyw, r2
/*   60   */          dp4 o2.___w, c15.zxyw, r2
/*   61   */          rsq r0.x___, r_abs[0].x
/*   62   */          mul r0.xyz_, r0.yzww, r0.xxxx
/*    6.1 */       exece
/*   63   */          dp3 o1.x___, r0.zyxx, c12.zxyy
/*   64   */          dp3 o1._y__, r0.zyxx, c13.zxyy
/*   65   */          dp3 o1.__z_, r0.zyxx, c14.zxyy
